"""
send a 28x28 camera image to the STM32F407G-DISC1
through the ST-LINK debug cable.

Install once:pip install pyocd numpy opencv-python

"""
import argparse #reads options
import struct #pack numbers into raw bytes
import sys #used to exit if no ST-LINK found
import time #used for delays

import numpy as np #hold images in array

RAM_START = 0x20000000 #ARM put RAM at this memory location
RAM_SIZE = 128*1024 # (128Kbytes)
# struct.pack turns numbers into raw bytes
# < is for little endian
# store those numbers "MNIS TBOX" as nametag for mailbox
TAG = struct.pack("<II", 0x4D4E4953, 0x54424F58) 

"""
Mailbox layout in STM RAM (offset = bytes from start of mailbox, mb)
    0   magic1   "MNIS" (tag part 1)
    4   magic2   "BOX!" (tag part 2)
    8   button   STM sets 1 when blue button pressed, PC clears to 0
    12  cmd      0 = idle, 1 = image ready (PC), 2 = done (STM)
    16  result   predicted digit (0-9)
    20  img      784 pixels (28x28), 1 byte each
"""


# ---------- connection ----------

def connect(): #open link to ST-LINK
    from pyocd.core.helpers import ConnectHelper #tool from pyocd which finds debug probes
    session = ConnectHelper.session_with_chosen_probe( #choose probe
        target_override="cortex_m", # generic Cortex-M
        connect_mode="attach",        # don't pause the running program
        blocking=False, #if no probe...dont wait for one...return nothing
    )
    if session is None:
        print("No ST-LINK found. Is the board plugged in?")
        sys.exit(1)
    session.open() #open the session
    return session # Return since it's a local variable

def find_mailbox(target): #search RAM for tag
    chunk = 4096 #read 4Kbytes at a time
    for addr in range(RAM_START, RAM_START + RAM_SIZE, chunk):
        data = bytes(target.read_memory_block8(addr, chunk))
        i = data.find(TAG)
        if i >= 0 and (i % 4) == 0: #modulus checks that address is multiple of 4
            return addr + i #actual adress in RAM
    return None




def send_image(target, mb, img): #target=STM, mb=mailbox address, img=28x28 image

    #dtype=np.unit8 makes every pixel an int between 0-255 (uint8 does this)
    # .reshape(784) changes 28x28 into a single row of 784
    data = np.asarray(img, dtype=np.uint8).reshape(784)

    #put image into STM RAM (mb + OFF_IMG) is the address of the image in RAM -> mb=mailbox, 20=location of image in mailbox
    #since pyOCD expects a list, .tolist converts an array to a list
    target.write_memory_block8(mb + 20, data.tolist()) 

    #write 1 to the command location in the mailbox (1 means image written, 0 means idle, 2 means STM is done)
    target.write32(mb + 12, 1)

    t0 = time.time() #save current time (seconds)
    while target.read32(mb + 12) != 2: # wait until the STM is done
        if time.time() - t0 > 2: #if 2 seconds passed...(too long)
            print("ERR: STM didn't answer within 2 s")
            return None
        time.sleep(0.005) #pause for 5 ms before checking agian

    #read STM answer out of mailbox (located 16 bytes into mailbox)
    result = target.read32(mb + 16)
    target.write32(mb + 12, 0) #write 0 into command (set to idle)

    print(f"STM result={result}") #print the result in the terminal (for troubleshooting)
    return result



# ---------- transform image into MNIST-style 28x28 ----------
# need a white digit on black background (28x28 pixels)

def center_roi(frame): #crop image into square
    h, w = frame.shape[:2] #get the frames height and width (whole image...rectangle)
    s = int(min(h, w)) #get the side length of square that is cut...use the smaller side for square length

    #x0 = (total width) - (square width)... //2 cuts off the remainder to make it a whole number
    # y0 is the same idea but top to bottom
    x0, y0 = (w - s) // 2, (h - s) // 2 #find the top left corner
    return frame[y0:y0 + s, x0:x0 + s] # return cropped square 


def binarize(roi_bgr): #turn color image into white image on black background
    import cv2 #import openCV 
    gray = cv2.cvtColor(roi_bgr, cv2.COLOR_BGR2GRAY) #convert the color of roi_bgr to grayscale
    gray = cv2.GaussianBlur(gray, (5, 5), 0) #soften image to remove any noise

    #find the average brightness of pixels in area. If the pixel is darker than it's neighbors, it's ink so it becomes white
    bw = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 51, 15)

    # remove tiny white specs (shrink white area then grow back...removes all specs)
    bw = cv2.morphologyEx(bw, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))  # remove specks
    return bw #return the clean black and white image


def preprocess(roi_bgr): #turn black and white image into 28x28 image
    import cv2 #import openCV
    bw = binarize(roi_bgr) #function created above...stores white/black image

    H, W = bw.shape #number of rows and columns (height and width of image)

    #n=how many white blobs there are
    #labels gives a number for each blob
    # stats is a table with one row per blob: 
        #x,y (top left corner of box)
        #w, h (box width and height)
        #aread (how many pixels in blob)
    n, labels, stats, _ = cv2.connectedComponentsWithStats(bw, 8) #find separate white blobs

    best, best_area = 0, 0 #start at 0,0 
    for i in range(1, n): #go through every blob. skips 1 (label for background)
        x, y, w, h, area = stats[i] #pull the stats and assign

        #true if the blob box reachs any side of the image (check for shadows/papers edge/cutoff digit/etc)
        touches_edge = x <= 1 or y <= 1 or x + w >= W - 1 or y + h >= H - 1

        #if this touches the edge, or is smaller than 0.1% of whole image(assumed that it's a spec)...it is skipped
        if touches_edge or area < 0.001 * H * W:
            continue #skip this blob

        if area > best_area: #assign newest best
            best, best_area = i, area
    if best == 0:
        return None

    # include nearby pieces too using the main blob's box grown by 30%
    x, y, w, h, _ = stats[best] #get the box of the best blob
    gx, gy = int(0.3 * w), int(0.3 * h) #grow box by 30%
    x0, y0 = max(0, x - gx), max(0, y - gy) #move top left corner by that amount...kept inside image
    x1, y1 = min(W, x + w + gx), min(H, y + h + gy) #move bottom right corner...kept inside image
    keep = np.zeros_like(bw) #make a black image the same size as our image...(blank canvas for number)

    for i in range(1, n): #go through every blob skipping background
        cx, cy, cw, ch, area = stats[i] #pull out blobs stats
        inside = cx >= x0 and cy >= y0 and cx + cw <= x1 and cy + ch <= y1 #check if in the new box size
        if i == best or (inside and area >= 0.0005 * H * W):
            keep[labels == i] = 255 #keep and make white

    ys, xs = np.nonzero(keep) #find every white pixel and give the row and column numbers
    digit = keep[ys.min():ys.max() + 1, xs.min():xs.max() + 1] #crop the rectangle with white pixels
    h, w = digit.shape #get the height and width of cropped image

    # thicken thin strokes so they end up ~2.5 px wide at 28x28, like MNIST

    #find countours traces outline of white shape
    contours, _ = cv2.findContours(digit, cv2.RETR_LIST, cv2.CHAIN_APPROX_NONE) 

    #total the length of all the outlines
    perimeter = sum(cv2.arcLength(c, True) for c in contours)

    #estimate how thick the penwidth is
    stroke = 2.0 * np.count_nonzero(digit) / max(perimeter, 1.0)   # current width (px)

    #target how thick the stroke should be
    target = 2.5 * max(w, h) / 20.0                                  # wanted width (px)

    if stroke < target: #thicken the stroke
        k = int(round(target - stroke)) + 1
        digit = cv2.copyMakeBorder(digit, k, k, k, k, cv2.BORDER_CONSTANT, value=0)
        digit = cv2.dilate(digit, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (k, k))) #make strokes thicker
        h, w = digit.shape #update shape size

    scale = 20.0 / max(w, h) #shrink so the longer side is 20 pixels (this is what MNIST does too...its a 28x28 but in 20x20 size)
    
    nw, nh = max(1, round(w * scale)), max(1, round(h * scale)) #new width and height of the digit after shrinking

    digit = cv2.resize(digit, (nw, nh), interpolation=cv2.INTER_AREA) #shrink the digit to the new size (nw x nh)

    out = np.zeros((28, 28), np.uint8) #blank 28x28 black image (final canvas)

    ox, oy = (28 - nw) // 2, (28 - nh) // 2 #center the box

    out[oy:oy + nh, ox:ox + nw] = digit #paste the digit into middle of black canvas

    #moments finds the weighted sum...finding brightness balance
    m = cv2.moments(out) 

    if m["m00"] > 0: #move digit so the brightest portions are "centered by mass" ...MNIST did this too...
        cx, cy = m["m10"] / m["m00"], m["m01"] / m["m00"]
        shift = np.float32([[1, 0, 14 - cx], [0, 1, 14 - cy]])
        out = cv2.warpAffine(out, shift, (28, 28))

    return out #return finished product



# ---------- camera loop ----------

#target=STM, mb=mailbox, cam_index=camera used (0 for computer...1 for my USB camera)
def camera_loop(target, mb, cam_index): 
    import cv2 #import openCV

    #pick the camera driver
    backend = cv2.CAP_DSHOW if sys.platform.startswith("win") else cv2.CAP_ANY

    #open the camera
    cap = cv2.VideoCapture(cam_index, backend)

    #if camera doesnt open
    if not cap.isOpened():
        print(f"Could not open camera {cam_index}.")
        return

    target.write32(mb + 8, 0)#clear button flag

    #make one full screen window (left half = camera, right half = 28x28 digit)
    cv2.namedWindow("digit reader", cv2.WINDOW_NORMAL) #create a window that can be resized
    cv2.setWindowProperty("digit reader", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN) #make it full screen

    while True: #constantly read the value
        ok, frame = cap.read() #returns ok if image is taken. takes frame
        if not ok: # if image isnt taken
            print("Camera read failed") #print
            break #break

        roi = center_roi(frame) #cropped image into square
        img = preprocess(roi) #processed image

        # ----- left half: camera with green square (the area that gets processed) -----
        h, w = frame.shape[:2] #frame height and width
        s = min(h, w) #side of the square (same as center_roi)
        x0, y0 = (w - s) // 2, (h - s) // 2 #top left corner of the square
        view = frame.copy() #copy so the green square isnt drawn on the real frame
        cv2.rectangle(view, (x0, y0), (x0 + s - 1, y0 + s - 1), (0, 255, 0), 3) #green square, 3 px thick

        nh = 840 * h // w #new height when the camera is made 840 wide (keeps its shape)
        view = cv2.resize(view, (840, nh)) #make camera 840 wide
        left = np.zeros((840, 840, 3), np.uint8) #black 840x840 color canvas
        top = (840 - nh) // 2 #space above the camera so it is centered
        left[top:top + nh, :] = view #paste the camera into the middle of the canvas

        # ----- right half: the 28x28 digit (black if no digit found) -----
        if img is not None:
            right = cv2.resize(img, (840, 840), interpolation=cv2.INTER_NEAREST) #28x28 -> 840x840 (sharp pixels)
        else:
            right = np.zeros((840, 840), np.uint8) #nothing found...show black
        right = cv2.cvtColor(right, cv2.COLOR_GRAY2BGR) #gray -> color so it can sit next to the camera

        # ----- put the two halves side by side and show them -----
        cv2.imshow("digit reader", np.hstack((left, right))) #hstack = place left and right next to each other

        key = cv2.waitKey(1) & 0xFF #return key press
        trigger = key == ord(" ") #check if it was a space
        if target.read32(mb + 8): #check if it was the blue USER button
            target.write32(mb + 8, 0) #if it was, write 0 to clear button "flag"
            trigger = True #set trigger to 1

        if trigger: #if space or user button...
            if img is None:
                print("No digit found in the box")
            else:
                send_image(target, mb, img)  #send 28x28 image to STM
        if key == ord("q"): #if q...
            break #break

    cap.release() #close the camera
    cv2.destroyAllWindows() #clear the window

#main function
def main():
    ap = argparse.ArgumentParser() #create an argument called ap

    #ap will accept an option "--camera" then a number (to denote camera number)
    ap.add_argument("--camera", type=int)

    #read what was typed...store in args
    args = ap.parse_args()

    session = connect() #connect to STM (ST-LINK)

    try:
        target = session.target #find the STM connection
        mb = find_mailbox(target) # search for RAM tag
        if mb is None:
            print("Mailbox not found in RAM. Did you flash stm32_swd_mailbox.c and press RESET?")
            return
        if args.camera is not None: #check if camera number exists
            camera_loop(target, mb, args.camera) #run until q is pressed
    finally:
        session.close() #release ST-LINK so keil can use it


main() #run main
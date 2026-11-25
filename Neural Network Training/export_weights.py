# Creates: weights.h (has W1, b1, W2, and b2 as const float arrays)

import numpy as np #for arrays

w = np.load("weights.npz") #load the .npz file created in the feedforward network


def write_array(f, name, arr): #f=file (weight.h), name= C name (e.g. W1), arr is numpy array
    # flatten() uses row-major (C) order, so W1[i][j] ends up at W1[i*784 + j]
    #arr.astype(np.float32) converts every number into a 32-bit float
    #flatten turns the array into one row
    flat = arr.astype(np.float32).flatten()

    #creating C array
    f.write(f"static const float {name}[{flat.size}] = {{\n")

    #write in scientific notation (keep 8 digits)
    for k in range(0, flat.size, 8):  # 8 numbers per line
        # .8e always prints an exponent, so every literal is a valid C float (e.g. 1.0e+00f)
        f.write("    " + ", ".join(f"{v:.8e}f" for v in flat[k:k + 8]) + ",\n")
    
    f.write("};\n\n") #close the C array


# ---------- weights.h ----------
with open("weights.h", "w") as f:
    f.write("#define N_IN  784  // 28x28 pixels\n")
    f.write("#define N_HID 100  // hidden neurons\n")
    f.write("#define N_OUT 10   // digits 0-9\n\n")

    #write the const float array values for W1, b1, W2, b2
    for name in ["W1", "b1", "W2", "b2"]:
        write_array(f, name, w[name]) 

    #example C code: 
    """
    #define N_IN  784  // 28x28 pixels
    #define N_HID 100  // hidden neurons
    #define N_OUT 10   // digits 0-9

    static const float W1[78400] = {
    1.23000000e-02f, -4.51000000e-03f, 8.10000000e-02f, -2.20000000e-02f, 5.00000000e-03f, 3.31000000e-02f, -1.07000000e-02f, 6.60000000e-03f,
  
    .........
    };

    ....rest of code in .h file ......
    
    """
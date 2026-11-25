# import libraries
import pandas as pd # used for reading data
import numpy as np # used for linear algebra

train = np.array(pd.read_csv('mnist_train.csv')) #np.array makes a matrix and pd.read_csv opens the CSV file with the data
test = np.array(pd.read_csv('mnist_test.csv')) # used to test for overfitting (once training is done, test on new data it's never seen)


np.random.shuffle(train) #since they're stored in order, shuffle it

data_train = train.T # (785, 60000)
Y_train = data_train[0] # correct answers (labels)
X_train = data_train[1:785]/255 # neurons
#...each value is normalized betwen 0-1 since 255 is white and 0 is black

data_test = test.T # (785, 10000)
Y_test = data_test[0] # labels for test data
X_test = data_test[1:785]/255 #neurons

def init_params():
    W1 = np.random.rand(100, 784) -0.5 # W1 is size 100x784
    #creates random values between 0-1, then subtracts so it's [-0.5,0.5]
    #negative weights are needed so that every Z isnt positive. Otherwise, ReLU passes everything through (not activation)
    b1 = np.random.rand(100,1) -0.5 #b1 is size 100x1
    W2 = np.random.rand(10,100) - 0.5 #W2 is size 10x100
    b2 = np.random.rand(10,1) -0.5 #b2 is size 10x1
    return W1, b1, W2, b2

def ReLU(Z):
    return np.maximum(0,Z) 
    #since this is elementwise, if >0 ...return Z, if <0, return 0
    # Z is a matrix, and this compares each entry with 0 (element by element...keeping whichever is bigger)

def softmax(Z):
    # used to give values between 0-1 (for probabilities)
    # use the softmax equation, along with numpy versions of exponential (so that it does it to each element)
    return np.exp(Z)/np.sum(np.exp(Z), axis = 0, keepdims=True)

def forward_prop(W1, b1, W2, b2, X):
    Z1 = W1.dot(X) + b1
    A1 = ReLU(Z1)
    Z2 = W2.dot(A1) + b2
    A2 = softmax(Z2)
    return Z1, A1, Z2, A2

def one_hot(Y):
    one_hot_Y = np.zeros((Y.size, 10)) # make a matrix of Yx10 with all 0s
    # set the value to 1 at the correct row (increment up...0,1,2,etc), and column based on Y 
    # e.g. for Y = [3,0,1] 
    # result: 
    # 000100...
    # 100000...
    # 010000...
    one_hot_Y[np.arange(Y.size), Y] = 1 #give the rows numbers (e.g 0,1,2,...)
    one_hot_Y = one_hot_Y.T #transpose so that each column is an example
    return one_hot_Y

def deriv_ReLU(Z):
    # slope is 1 for x>0, 0 for x<=0
    return Z>0 #works since it's a boolean. returns 1 for Z>0 and 0 otherwise

def back_prop(Z1, A1, Z2, A2, W2, X, Y):
    one_hot_Y = one_hot(Y)
    dZ2 = A2 - one_hot_Y
    m = Y.size # m = number of columns
    dW2 = 1/m *dZ2.dot(A1.T)
    db2 = 1/m * np.sum(dZ2, axis=1, keepdims=True)
    dZ1 = W2.T.dot(dZ2) * deriv_ReLU(Z1)

    dW1 = 1/m *dZ1.dot(X.T)
    db1 = 1/m * np.sum(dZ1, axis=1,keepdims=True)
    return dW1, db1, dW2, db2

def update_params(W1, b1, W2, b2, dW1, db1, dW2, db2, alpha):
    W1 = W1 - alpha*dW1
    b1 = b1 - alpha*db1
    W2 = W2 - alpha*dW2
    b2 = b2 - alpha*db2
    return W1, b1, W2, b2

def get_predictions(A2):
    return np.argmax(A2, 0)

def get_accuracy(predictions, Y):
    return np.sum(predictions == Y) / Y.size

"""
def gradient_descent(X,Y, interations, alpha):
    W1, b1, W2, b2 = init_params() # load in the initial values
    for i in range(interations): # run loop
        Z1, A1, Z2, A2 = forward_prop(W1, b1, W2, b2, X)
        dW1, db1, dW2, db2 = back_prop(Z1, A1, Z2, A2, W2, X, Y)
        W1, b1, W2, b2 = update_params(W1, b1, W2, b2, dW1, db1, dW2, db2, alpha)
        if i % 50 == 0 or i == interations - 1: #print accuracy for every 50th iteration and last (499th) iteration
            print("Iteration:", i)
            print("Accuracy:", get_accuracy(get_predictions(A2), Y))
    return W1, b1, W2, b2
"""

def mini_batch_gradient_descent(X, Y, X_test, Y_test, epochs, alpha, batch_size):
    W1, b1, W2, b2 = init_params() # load in the initial values
    m_total = Y.size  # number training samples

    for epoch in range(epochs):  # epoch = one complete pass through the entire dataset
        # Shuffle the data at the start of each epoch so the model doesn't learn the order of the data
        order = np.random.permutation(m_total)
        X_shuffled = X[:, order]
        Y_shuffled = Y[order]

        #Split the data into mini-batches
        for k in range(0, m_total, batch_size): #start at 0...end at number of training samples...step by batch size
            X_batch = X_shuffled[:, k:k+batch_size] #all rows, select columns
            Y_batch = Y_shuffled[k:k+batch_size] #row of labels

            Z1, A1, Z2, A2 = forward_prop(W1, b1, W2, b2, X_batch) # compute the predictions for this mini-batch

            dW1, db1, dW2, db2 = back_prop(Z1, A1, Z2, A2, W2, X_batch, Y_batch)

            
            W1, b1, W2, b2 = update_params(W1, b1, W2, b2, dW1, db1, dW2, db2, alpha)# Update the parameters

            #Repeat for all mini-batches....

        #once epoch done...check accuracy on the test set then reshuffle and start the next epoch
        _, _, _, A2_test = forward_prop(W1, b1, W2, b2, X_test)
        print("Epoch", epoch, "test accuracy:", get_accuracy(get_predictions(A2_test), Y_test))

    return W1, b1, W2, b2


# ------------------Train--------------------
#W1, b1, W2, b2 = gradient_descent(X_train, Y_train, 500, 0.10)
W1, b1, W2, b2 = mini_batch_gradient_descent(X_train, Y_train, X_test, Y_test, 10,0.1, 64)
_, _, _, A2_test = forward_prop(W1, b1, W2, b2, X_test)
print("Test accuracy:", get_accuracy(get_predictions(A2_test), Y_test))

np.savez("weights.npz", W1=W1, b1=b1, W2=W2, b2=b2) #since the weights/biases are matrices, this was the 
#way I found to save them. This is a zip file and within are .npy files. I later convert these to .h


import ctypes
import threading
from flask import Flask, jsonify, request

blockchain = ctypes.CDLL('../libcore_capi.so')
blockchain.init.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
blockchain.init.restype = None
blockchain.getBalance.argtypes = [ctypes.c_char_p]
blockchain.getBalance.restype = ctypes.c_char_p
blockchain.getPendingBalance.argtypes = [ctypes.c_char_p]
blockchain.getPendingBalance.restype = ctypes.c_char_p
blockchain.getTransactions.argtypes = [ctypes.c_char_p]
blockchain.getTransactions.restype = ctypes.c_char_p
blockchain.getPendingTransactions.argtypes = [ctypes.c_char_p]
blockchain.getPendingTransactions.restype = ctypes.c_char_p
blockchain.getPendingTransactionsForAddress.argtypes = [ctypes.c_char_p]
blockchain.getPendingTransactionsForAddress.restype = ctypes.c_char_p
blockchain.getTransactionAmount.argtypes = [ctypes.c_char_p]
blockchain.getTransactionAmount.restype = ctypes.c_char_p
blockchain.getTransactionTime.argtypes = [ctypes.c_char_p]
blockchain.getTransactionTime.restype = ctypes.c_char_p
blockchain.getTransactionSender.argtypes = [ctypes.c_char_p]
blockchain.getTransactionSender.restype = ctypes.c_char_p
blockchain.getTransactionRecipient.argtypes = [ctypes.c_char_p]
blockchain.getTransactionRecipient.restype = ctypes.c_char_p
blockchain.createTransaction.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
blockchain.createTransaction.restype = ctypes.c_char_p
blockchain.sendTransaction.argtypes = [ctypes.c_char_p]
blockchain.sendTransaction.restype = ctypes.c_char_p

app = Flask(__name__)

@app.route('/getBalance', methods=['GET'])
def getBalance():
    address = request.args.get('address', default="")
    value = blockchain.getBalance(address.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getPendingBalance', methods=['GET'])
def getPendingBalance():
    address = request.args.get('address', default="")
    value = blockchain.getPendingBalance(address.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getTransactions', methods=['GET'])
def getTransactions():
    address = request.args.get('address', default="")
    transactions = blockchain.getTransactions(address.encode('utf-8')).decode('utf-8')
    data = {"transactions": transactions}
    return jsonify(data)

@app.route('/getPendingTransactions', methods=['GET'])
def getPendingTransactions():
    transactions = blockchain.getPendingTransactions().decode('utf-8')
    data = {"transactions": transactions}
    return jsonify(data)

@app.route('/getPendingTransactionsForAddress', methods=['GET'])
def getPendingTransactionsForAddress():
    address = request.args.get('address', default="")
    transactions = blockchain.getPendingTransactionsForAddress(address.encode('utf-8')).decode('utf-8')
    data = {"transactions": transactions}
    return jsonify(data)

@app.route('/getTransactionAmount', methods=['GET'])
def getTransactionAmount():
    hash = request.args.get('transactionHash', default="")
    value = blockchain.getTransactionAmount(hash.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getTransactionTime', methods=['GET'])
def getTransactionTime():
    hash = request.args.get('transactionHash', default="")
    value = blockchain.getTransactionTime(hash.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getTransactionSender', methods=['GET'])
def getTransactionSender():
    hash = request.args.get('transactionHash', default="")
    value = blockchain.getTransactionSender(hash.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getTransactionRecipient', methods=['GET'])
def getTransactionRecipient():
    hash = request.args.get('transactionHash', default="")
    value = blockchain.getTransactionRecipient(hash.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/createTransaction', methods=['GET'])
def createTransaction():
    sender = request.args.get('sender', default="")
    recipient = request.args.get('recipient', default="")
    amount = request.args.get('amount', default="0")
    fee = request.args.get('fee', default="0")
    type = request.args.get('type', default="")
    value = blockchain.createTransaction(
        sender.encode('utf-8'),
        recipient.encode('utf-8'),
        amount.encode('utf-8'),
        fee.encode('utf-8'),
        type.encode('utf-8')
        ).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/sendTransaction', methods=['GET'])
def sendTransaction():
    txData = request.args.get('data', default="")
    value = blockchain.sendTransaction(txData.encode('utf-8')).decode('utf-8')
    data = {"value": value}
    return jsonify(data)

@app.route('/getTransactionOverview', methods=['GET'])
def getTransactionOverview():
    address = request.args.get('address', default="")
    
    tx1 = blockchain.getPendingTransactionsForAddress(address.encode('utf-8')).decode('utf-8')
    tx2 = blockchain.getTransactions(address.encode('utf-8')).decode('utf-8')
    
    tx = []
    for i in tx1.split("\n"):
        if i != "":
            tx.append(i)
    pendingNum = len(tx)
    for i in reversed(tx2.split("\n")):
        if i != "":
            tx.append(i)
    
    num = 0
    data = []
    for i in tx:
        amount = blockchain.getTransactionAmount(i.encode('utf-8')).decode('utf-8')
        time = blockchain.getTransactionTime(i.encode('utf-8')).decode('utf-8')
        sender = blockchain.getTransactionSender(i.encode('utf-8')).decode('utf-8')
        recipient = blockchain.getTransactionRecipient(i.encode('utf-8')).decode('utf-8')
        pending = num < pendingNum
        data.append({
            "hash":i,
            "amount":amount,
            "time":time,
            "sender":sender,
            "recipient":recipient,
            "pending":pending,
        })
        num += 1

    return jsonify(data)

def run():
    blockchain.init(b"./chain", b"./key/key.txt", b"entry.txt")

if __name__ == '__main__':
    thr = threading.Thread(target=run, args=(), kwargs={})
    thr.start()
    app.run(host='0.0.0.0')

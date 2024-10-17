
import ctypes
from flask import Flask, jsonify, request

blockchain = ctypes.CDLL('./core_capi.dll')
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

blockchain.init(b"./chain", b"./key/key.txt", b"entry.txt")

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
    address = request.args.get('address', default="")
    transactions = blockchain.getPendingTransactions(address.encode('utf-8')).decode('utf-8')
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

if __name__ == '__main__':
    app.run()

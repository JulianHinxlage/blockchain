//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "wallet/Wallet.h"
#include "util/util.h"
#include "util/log.h"

#include "QtWidgets/qpushbutton.h"
#include "QtWidgets/qapplication.h"
#include "QtWidgets/qlabel.h"
#include "QtWidgets/qboxlayout.h"
#include "QtWidgets/qtextedit.h"
#include "qthread.h"
#include "qscrollbar.h"
#include "qdialog.h"
#include "qlineedit.h"
#include "qscrollarea.h"
#include "qtimer.h"


#include <filesystem>

#if WIN32
#include <Windows.h>
#include <Psapi.h>
#include <Winternl.h>
#undef max

int processCount(const std::string& name) {
	int count = 0;
	DWORD processesArray[1024];
	DWORD bytesReturned;
	if (EnumProcesses(processesArray, sizeof(processesArray), &bytesReturned)) {
		int numProcesses = bytesReturned / sizeof(DWORD);

		for (int i = 0; i < numProcesses; ++i) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processesArray[i]);
			if (hProcess != NULL) {
				char processName[MAX_PATH];
				if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
					std::string pname = processName;
					if (pname == name) {
						count++;
					}
				}

				CloseHandle(hProcess);
			}
		}
	}

	return count;
}
#else
int processCount(const std::string& name) {
	return 1;
}
#endif

std::string search(std::string path) {
	std::string orig = path;
	for (int i = 0; i < 3; i++) {
		if (std::filesystem::exists(path)) {
			return path;
		}
		path = "../" + path;
	}
	if (std::filesystem::is_directory(orig)) {
		std::filesystem::create_directories(orig);
	}
	else {
		std::string parent = std::filesystem::path(orig).parent_path().string();
		if (!parent.empty()) {
			std::filesystem::create_directories(std::filesystem::path(orig).parent_path());
		}
	}
	return orig;
}

class ConfirmPopup : public QDialog {
public:
	ConfirmPopup(const std::string& msg, QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle("Confirm");
		setFixedSize(400, 200);

		QVBoxLayout* layout = new QVBoxLayout(this);

		QLabel* msgLable = new QLabel(msg.c_str(), this);
		layout->addWidget(msgLable);

		QPushButton* yes = new QPushButton("Yes", this);
		layout->addWidget(yes);
		connect(yes, &QPushButton::clicked, this, &ConfirmPopup::accept);

		QPushButton* no = new QPushButton("No", this);
		no->setFocus();
		layout->addWidget(no);
		connect(no, &QPushButton::clicked, this, &ConfirmPopup::reject);
	}
};

class MessagePopup : public QDialog {
public:
	MessagePopup(const std::string &msg, QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle("Message");
		setFixedSize(400, 200);

		QVBoxLayout* layout = new QVBoxLayout(this);

		QLabel* msgLable = new QLabel(msg.c_str(), this);
		layout->addWidget(msgLable);

		QPushButton* close = new QPushButton("Close", this);
		close->setFocus();
		layout->addWidget(close);
		connect(close, &QPushButton::clicked, this, &MessagePopup::accept);
	}
};

class PasswordPopup : public QDialog {
public:
	std::string password;

	PasswordPopup(QWidget* parent = nullptr) : QDialog(parent) {
		password = "";

		setWindowTitle("Enter Password");
		setFixedSize(400, 200);

		QVBoxLayout* layout = new QVBoxLayout(this);


		QLabel* label = new QLabel("Password:", this);
		QLineEdit* psw = new QLineEdit();
		psw->setEchoMode(QLineEdit::Password);
		layout->addWidget(label);
		layout->addWidget(psw);


		QPushButton* ok = new QPushButton("Ok", this);
		layout->addWidget(ok);
		QObject::connect(ok, &QPushButton::clicked, [&, psw]() {
			password = psw->text().toStdString();
			accept();
		});

		QPushButton* close = new QPushButton("Close", this);
		close->setFocus();
		layout->addWidget(close);
		connect(close, &QPushButton::clicked, this, &PasswordPopup::reject);
	}
};

class SendPopup : public QDialog {
public:
	SendPopup(Wallet* wallet, QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle("Send");
		setFixedSize(600, 200);

		QVBoxLayout* layout = new QVBoxLayout(this);


		QLabel* recipientLabel = new QLabel("Recipient:", this);
		layout->addWidget(recipientLabel);

		QLineEdit* recipient = new QLineEdit();
		layout->addWidget(recipient);


		QLabel* amountLabel = new QLabel("Amount:", this);
		layout->addWidget(amountLabel);

		QLineEdit* amount = new QLineEdit();
		layout->addWidget(amount);

		QPushButton* send = new QPushButton("Send", this);
		layout->addWidget(send);
		QObject::connect(send, &QPushButton::clicked, [&, wallet, recipient, amount]() {
			if (wallet->keyStore.isLocked()) {
				wallet->keyStore.unlock("");
			}

			if (wallet->keyStore.isLocked()) {
				PasswordPopup popup;
				if (popup.exec() != QDialog::Accepted) {
					return;
				}
				if (!wallet->keyStore.unlock(popup.password)) {
					MessagePopup popup("Invalid Password");
					popup.exec();
					return;
				}
			}

			wallet->sendTransaction(recipient->text().toStdString(), amount->text().toStdString(), "0");
			wallet->keyStore.lock();
			accept();
		});

		QPushButton* close = new QPushButton("Close", this);
		close->setFocus();
		layout->addWidget(close);
		connect(close, &QPushButton::clicked, this, &SendPopup::reject);
	}
};

class ChangePasswordPopup : public QDialog {
public:
	ChangePasswordPopup(Wallet* wallet, QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle("Change Password");
		setFixedSize(600, 240);

		QVBoxLayout* layout = new QVBoxLayout(this);

		QLineEdit* old = nullptr;
		if (wallet->keyStore.isLocked()) {
			wallet->keyStore.unlock("");
		}
		if (wallet->keyStore.isLocked()) {
			QLabel* oldLabel = new QLabel("Old Password:", this);
			old = new QLineEdit();
			old->setEchoMode(QLineEdit::Password);
			layout->addWidget(oldLabel);
			layout->addWidget(old);
		}


		QLabel* newLable = new QLabel("New Password:", this);
		layout->addWidget(newLable);
		QLineEdit* newPsw = new QLineEdit();
		newPsw->setEchoMode(QLineEdit::Password);
		layout->addWidget(newPsw);


		QLabel* new2Lable = new QLabel("Confirm Password:", this);
		layout->addWidget(new2Lable);
		QLineEdit* newPsw2 = new QLineEdit();
		newPsw2->setEchoMode(QLineEdit::Password);
		layout->addWidget(newPsw2);


		QPushButton* apply = new QPushButton("Apply", this);
		layout->addWidget(apply);
		QObject::connect(apply, &QPushButton::clicked, [&, wallet, old, newPsw, newPsw2]() {
			if (wallet->keyStore.isLocked()) {
				wallet->keyStore.unlock("");
			}

			if (wallet->keyStore.isLocked()) {
				if (!old || !wallet->keyStore.unlock(old->text().toStdString())) {
					wallet->keyStore.lock();
					MessagePopup popup("Invalid Password");
					popup.exec();
					return;
				}
			}

			if (newPsw->text().toStdString() != newPsw2->text().toStdString()) {
				wallet->keyStore.lock();
				MessagePopup popup("Password is not the same");
				popup.exec();
				return;
			}

			if (newPsw->text().toStdString().empty()) {
				wallet->keyStore.lock();
				ConfirmPopup popup("Do you want to remove the password?");
				if (popup.exec() != QDialog::Accepted) {
					return;
				}
			}

			wallet->keyStore.setPassword(newPsw->text().toStdString());
			wallet->keyStore.lock();
			log(LogLevel::INFO, "KeyStore", "the password was changed");
			accept();
		});

		QPushButton* close = new QPushButton("Close", this);
		close->setFocus();
		layout->addWidget(close);
		connect(close, &QPushButton::clicked, this, &ChangePasswordPopup::reject);
	}
};

class TransactionListEntry : public QWidget {
public:
	TransactionListEntry(QWidget* parent = nullptr) : QWidget(parent) {
		QVBoxLayout* layout = new QVBoxLayout(this);
		QLabel* titleLabel = new QLabel("Title", this);
		QPushButton* actionButton = new QPushButton("Action", this);

		layout->addWidget(titleLabel);
		layout->addWidget(actionButton);
	}
};

class TransactionList : public QWidget {
public:
	QVBoxLayout* layout = nullptr;
	QScrollArea* scrollArea = nullptr;
	QWidget* scrollWidget = nullptr;
	QVBoxLayout* scrollLayout = nullptr;
	QTimer* timer = nullptr;

	std::vector<QWidget*> subWidgets;
	Wallet* wallet;

	TransactionList(Wallet *wallet, QWidget* parent = nullptr) : QWidget(parent) {
		this->wallet = wallet;
		layout = new QVBoxLayout(this);

		scrollArea = new QScrollArea(this);
		scrollWidget = new QWidget();
		scrollLayout = new QVBoxLayout(scrollWidget);
		scrollWidget->setLayout(scrollLayout);
		scrollArea->setWidget(scrollWidget);
		scrollArea->setWidgetResizable(true);

		
		layout->addWidget(scrollArea);
		setLayout(layout);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &TransactionList::updateList);
		timer->start(1000);
	}

	~TransactionList() {
		timer->stop();
		clear();
		delete scrollLayout;
		delete scrollWidget;
		delete scrollArea;
		delete layout;
		delete timer;
	}

	void clear() {
		for (auto& sub : subWidgets) {
			scrollLayout->removeWidget(sub);
			delete sub;
		}
		subWidgets.clear();
	}

	std::string dateAndTime(uint32_t time) {
		time_t rawtime = time;
		struct tm* timeinfo;
		char buffer[80];

		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", timeinfo);
		return std::string(buffer);
	}

	void add(Amount amount, EccPublicKey counterpart, bool negative, uint64_t time, TransactionType type, bool pending) {
		QWidget* entryWidget = new QWidget(scrollWidget);
		QHBoxLayout* layout = new QHBoxLayout(entryWidget);
		
		if (negative) {
			QLabel* label = new QLabel(("-" + amountToCoin(amount)).c_str(), entryWidget);
			label->setStyleSheet("color: red");
			layout->addWidget(label);
		}
		else {
			QLabel* label = new QLabel(amountToCoin(amount).c_str(), entryWidget);
			label->setStyleSheet("color: green");
			layout->addWidget(label);
		}

		{
			QLabel* label = new QLabel(dateAndTime(time).c_str(), entryWidget);
			layout->addWidget(label);
		}

		{
			QLabel* label = new QLabel(pending ? "PENDING" : "", entryWidget);
			if (pending) {
				label->setStyleSheet("color: red");
			}
			layout->addWidget(label);
		}


		{
			QLabel* label = new QLabel(toHex(counterpart).c_str(), entryWidget);
			layout->addWidget(label);
		}


		entryWidget->setLayout(layout);
		scrollLayout->insertWidget(0, entryWidget);
		subWidgets.push_back(entryWidget);
	}

	void updateList() {
		clear();

		int count = wallet->node.blockChain.getBlockCount();
		for (int i = 0; i < count; i++) {
			Hash hash = wallet->node.blockChain.getBlockHash(i);
			Block block = wallet->node.blockChain.getBlock(hash);
			for (auto& txHash : block.transactionTree.transactionHashes) {
				Transaction tx = wallet->node.blockChain.getTransaction(txHash);

				bool isSender = tx.header.sender == wallet->keyStore.getPublicKey();
				bool isRecipient = tx.header.recipient == wallet->keyStore.getPublicKey();
				if (isSender || isRecipient) {
					if (isSender) {
						
						add(tx.header.amount, tx.header.recipient, true, tx.header.timestamp, tx.header.type, false);
					}
					else {
						add(tx.header.amount, tx.header.sender, false, tx.header.timestamp, tx.header.type, false);
					}
				}
			}
		}

		for (auto& hash : wallet->node.blockChain.getPendingTransactions()) {
			Transaction tx = wallet->node.blockChain.getTransaction(hash);

			bool isSender = tx.header.sender == wallet->keyStore.getPublicKey();
			bool isRecipient = tx.header.recipient == wallet->keyStore.getPublicKey();
			if (isSender || isRecipient) {
				if (isSender) {
					add(tx.header.amount, tx.header.recipient, true, tx.header.timestamp, tx.header.type, true);
				}
				else {
					add(tx.header.amount, tx.header.sender, false, tx.header.timestamp, tx.header.type, true);
				}
			}
		}
	}

};


const std::string stateStr(FullNodeState state) {
	switch (state)
	{
	case INIT:
		return "INIT";
	case RUNNING:
		return "RUNNING";
	case VERIFY_CHAIN:
		return "VERIFY_CHAIN";
	case SYNCHRONISING_FETCH:
		return "SYNCHRONISING_FETCH";
	case SYNCHRONISING_VERIFY:
		return "SYNCHRONISING_VERIFY";
	default:
		return "UNKNOWN";
	}
}




int main(int argc, char* argv[]) {
	Wallet wallet;


    QApplication app(argc, argv);

	QWidget mainWindow;
	mainWindow.resize(800, 600);
	mainWindow.setWindowTitle("Wallet");

	QVBoxLayout* mainLayout = new QVBoxLayout(&mainWindow);
	mainWindow.setLayout(mainLayout);

	QLabel status("Status: UNKNOWN", &mainWindow);
	QLabel address("Address: ...", &mainWindow);
	QLabel balance("Balance: ...", &mainWindow);

	status.setTextInteractionFlags(Qt::TextSelectableByMouse);
	address.setTextInteractionFlags(Qt::TextSelectableByMouse);
	balance.setTextInteractionFlags(Qt::TextSelectableByMouse);

	mainLayout->addWidget(&status);
	mainLayout->addWidget(&address);
	mainLayout->addWidget(&balance);
	
	
	
	QPushButton send("Send", &mainWindow);
	mainLayout->addWidget(&send);
	QObject::connect(&send, &QPushButton::clicked, [&]() {
		SendPopup popup(&wallet);
		popup.exec();
	});


	QPushButton change("Change Password", &mainWindow);
	mainLayout->addWidget(&change);
	QObject::connect(&change, &QPushButton::clicked, [&]() {
		ChangePasswordPopup popup(&wallet);
		popup.exec();
	});


	TransactionList trasnactions(&wallet, &mainWindow);
	mainLayout->addWidget(&trasnactions);
	
	//mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));



	QTextEdit log;
	log.setReadOnly(true);
	std::string logText;
	std::mutex logMutex;
	setLogCallback([&](LogLevel level, const std::string& category, const std::string& msg) {
		std::unique_lock<std::mutex> lock(logMutex);
		if (!logText.empty()) {
			logText += "\n";
		}
		logText += msg;
	});
	mainLayout->addWidget(&log);


	std::atomic_bool running = true;

	std::thread* initThread = new std::thread([&]() {
		int num = 0;
		std::string name = "name";
		if (argc > 0) {
			std::string exe = argv[0];
			name = std::filesystem::path(exe).stem().string();
			num = processCount(std::filesystem::path(exe).filename().string());
		}
		std::string chainDir = search("chains") + "/" + name + "/chain_" + std::to_string(num);
		std::string keyFile = search("keys") + "/" + name + "/key_" + std::to_string(num) + ".txt";
		wallet.init(chainDir, keyFile, search("entry.txt"));
	});

	std::thread* thread = new std::thread([&]() {
		while (running) {
			status.setText(("Status: " + stateStr(wallet.node.getState())).c_str());
			address.setText(("Address: " + toHex(wallet.keyStore.getPublicKey())).c_str());
			if (wallet.initialized) {
				Amount pending = wallet.getPendingBalance();
				if (pending == 0) {
					balance.setText(("Balance: " + amountToCoin(wallet.getAccount().balance)).c_str());
				}
				else {
					if (pending < Amount(1) << 63) {
						balance.setText(("Balance: " + amountToCoin(wallet.getAccount().balance) + " (+" + amountToCoin(pending) + ")").c_str());
					}
					else {
						pending = -pending;
						balance.setText(("Balance: " + amountToCoin(wallet.getAccount().balance) + " (-" + amountToCoin(pending) + ")").c_str());
					}
				}
			}
			else {
				balance.setText("Balance: ...");
			}
			{
				std::unique_lock<std::mutex> lock(logMutex);
				if (!logText.empty()) {
					QMetaObject::invokeMethod(&log, "append", Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(logText)));
					logText = "";
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});

	mainWindow.show();

    app.exec();

	running = false;
	initThread->join();
	thread->join();
	delete thread;
	delete initThread;
	return 0;
}

/*
 * ==============================================================
 *          BANK MANAGEMENT APPLICATION  -  C++
 *    OOP  |  File Handling  |  Menu-Driven  |  Secure PIN
 * ==============================================================
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <limits>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// ==============================================================
//  UTILITY  -  Terminal helpers
// ==============================================================
namespace UI {

void clear() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void border(char c = '=', int w = 62) { cout << "  " << string(w, c) << "\n"; }

void title(const string& t) {
    border();
    int pad = max(0, (58 - (int)t.size()) / 2);
    cout << "  |" << string(pad, ' ') << t
         << string(max(0, 58 - pad - (int)t.size()), ' ') << "|\n";
    border();
}

void success(const string& msg) { cout << "\n  [OK]  " << msg << "\n"; }
void error  (const string& msg) { cout << "\n  [ERR] " << msg << "\n"; }
void info   (const string& msg) { cout << "\n  [>>]  " << msg << "\n"; }

string timestamp() {
    time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

} // namespace UI

// ==============================================================
//  INPUT  -  Validated input helpers
// ==============================================================
namespace Input {

int getInt(const string& prompt) {
    int v;
    while (true) {
        cout << prompt;
        if (cin >> v) { cin.ignore(); return v; }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        UI::error("Invalid input. Please enter a number.");
    }
}

double getDouble(const string& prompt, double minVal = 0.01) {
    double v;
    while (true) {
        cout << prompt;
        if (cin >> v && v >= minVal) { cin.ignore(); return v; }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        UI::error("Invalid amount. Must be >= " + to_string(minVal));
    }
}

string getString(const string& prompt) {
    string s;
    cout << prompt;
    getline(cin, s);
    return s;
}

string getPin(const string& prompt) {
    cout << prompt;
    string pin;
    getline(cin, pin);   // simple read; mask not possible portably
    return pin;
}

} // namespace Input

// ==============================================================
//  TRANSACTION  -  Immutable log entry
// ==============================================================
struct Transaction {
    string type;
    double amount;
    double balAfter;
    string note;
    string datetime;

    string serialize() const {
        ostringstream oss;
        oss << type << "|" << fixed << setprecision(2)
            << amount << "|" << balAfter << "|"
            << datetime << "|" << note;
        return oss.str();
    }

    static Transaction deserialize(const string& line) {
        Transaction t;
        istringstream iss(line);
        string tok;
        getline(iss, tok, '|'); t.type     = tok;
        getline(iss, tok, '|'); t.amount   = stod(tok);
        getline(iss, tok, '|'); t.balAfter = stod(tok);
        getline(iss, tok, '|'); t.datetime = tok;
        getline(iss, tok);      t.note     = tok;
        return t;
    }
};

// ==============================================================
//  ACCOUNT  -  Core entity (OOP)
// ==============================================================
class Account {
private:
    int    accNo;
    string holderName;
    string pin;
    string accType;       // Savings | Current
    double balance;
    vector<Transaction> txHistory;

    static const double MIN_BALANCE;

public:
    Account() : accNo(0), balance(0.0) {}

    Account(int no, const string& name, const string& pinCode,
            const string& type, double initBalance)
        : accNo(no), holderName(name), pin(pinCode),
          accType(type), balance(initBalance) {}

    int    getAccNo()   const { return accNo;      }
    string getName()    const { return holderName; }
    string getType()    const { return accType;    }
    double getBalance() const { return balance;    }
    const vector<Transaction>& getHistory() const { return txHistory; }

    bool verifyPin(const string& input) const { return pin == input; }

    bool deposit(double amount, const string& note = "Cash deposit") {
        if (amount <= 0) return false;
        balance += amount;
        txHistory.push_back({"DEPOSIT", amount, balance, note, UI::timestamp()});
        return true;
    }

    bool withdraw(double amount, const string& note = "Cash withdrawal") {
        if (amount <= 0 || balance - amount < MIN_BALANCE) return false;
        balance -= amount;
        txHistory.push_back({"WITHDRAW", amount, balance, note, UI::timestamp()});
        return true;
    }

    void creditTransfer(double amount, int fromAcc) {
        balance += amount;
        txHistory.push_back({"TRANSFER_IN", amount, balance,
                              "From Acc#" + to_string(fromAcc), UI::timestamp()});
    }

    bool debitTransfer(double amount, int toAcc) {
        if (amount <= 0 || balance - amount < MIN_BALANCE) return false;
        balance -= amount;
        txHistory.push_back({"TRANSFER_OUT", amount, balance,
                              "To Acc#" + to_string(toAcc), UI::timestamp()});
        return true;
    }

    void printSummary() const {
        UI::border('-');
        cout << "  Account No : " << accNo       << "\n"
             << "  Holder     : " << holderName   << "\n"
             << "  Type       : " << accType      << "\n"
             << "  Balance    : INR " << fixed << setprecision(2) << balance << "\n";
        UI::border('-');
    }

    void printStatement(int last = 10) const {
        printSummary();
        cout << "\n  Last " << last << " Transactions:\n";
        UI::border('-');
        cout << "  " << left
             << setw(14) << "Type"
             << setw(12) << "Amount"
             << setw(12) << "Balance"
             << setw(20) << "Date/Time"
             << "Note\n";
        UI::border('-');

        int start = max(0, (int)txHistory.size() - last);
        for (int i = start; i < (int)txHistory.size(); ++i) {
            const auto& t = txHistory[i];
            cout << "  " << left
                 << setw(14) << t.type
                 << setw(12) << fixed << setprecision(2) << t.amount
                 << setw(12) << t.balAfter
                 << setw(20) << t.datetime
                 << t.note << "\n";
        }
        if (txHistory.empty())
            UI::info("No transactions yet.");
        UI::border('-');
    }

    string serialize() const {
        ostringstream oss;
        oss << accNo << "\n"
            << holderName << "\n"
            << pin << "\n"
            << accType << "\n"
            << fixed << setprecision(2) << balance << "\n"
            << txHistory.size() << "\n";
        for (const auto& t : txHistory)
            oss << t.serialize() << "\n";
        return oss.str();
    }

    static Account deserialize(istream& in) {
        Account a;
        string line;
        getline(in, line); a.accNo      = stoi(line);
        getline(in, a.holderName);
        getline(in, a.pin);
        getline(in, a.accType);
        getline(in, line); a.balance    = stod(line);
        getline(in, line); int txCount  = stoi(line);
        for (int i = 0; i < txCount; ++i) {
            getline(in, line);
            a.txHistory.push_back(Transaction::deserialize(line));
        }
        return a;
    }
};

const double Account::MIN_BALANCE = 500.0;

// ==============================================================
//  BANK  -  Manages collection of accounts (OOP)
// ==============================================================
class Bank {
private:
    string bankName;
    string dataFile;
    vector<Account> accounts;
    int nextAccNo;

    void load() {
        ifstream fin(dataFile);
        if (!fin) return;
        string line;
        getline(fin, line); nextAccNo = stoi(line);
        getline(fin, line); int count = stoi(line);
        for (int i = 0; i < count; ++i)
            accounts.push_back(Account::deserialize(fin));
    }

    void save() const {
        ofstream fout(dataFile, ios::trunc);
        fout << nextAccNo << "\n" << accounts.size() << "\n";
        for (const auto& a : accounts)
            fout << a.serialize();
    }

    Account* find(int accNo) {
        for (auto& a : accounts)
            if (a.getAccNo() == accNo) return &a;
        return nullptr;
    }

    Account* authenticate(int accNo, const string& pin) {
        Account* a = find(accNo);
        if (!a)               { UI::error("Account not found.");  return nullptr; }
        if (!a->verifyPin(pin)) { UI::error("Incorrect PIN.");    return nullptr; }
        return a;
    }

public:
    Bank(const string& name, const string& file)
        : bankName(name), dataFile(file), nextAccNo(1001) {
        load();
    }

    ~Bank() { save(); }

    // ---- 1. Create Account ------------------------------------
    void createAccount() {
        UI::clear();
        UI::title("  OPEN NEW ACCOUNT  ");
        cout << "\n";

        string name = Input::getString("  Full Name     : ");
        string pin1 = Input::getPin   ("  Set PIN       : ");
        string pin2 = Input::getPin   ("  Confirm PIN   : ");
        if (pin1 != pin2) { UI::error("PINs do not match."); UI::pause(); return; }
        if (pin1.size() < 4) { UI::error("PIN must be at least 4 digits."); UI::pause(); return; }

        cout << "\n  Account Type:\n"
             << "    [1] Savings\n"
             << "    [2] Current\n";
        int ch = Input::getInt("  Choice: ");
        string type = (ch == 2) ? "Current" : "Savings";

        double initDep = Input::getDouble("  Initial Deposit (min INR 500): ", 500.0);

        Account a(nextAccNo++, name, pin1, type, 0.0);
        a.deposit(initDep, "Account opening deposit");
        accounts.push_back(a);
        save();

        UI::success("Account created! Your Account No: " + to_string(a.getAccNo()));
        UI::pause();
    }

    // ---- 2. Deposit -------------------------------------------
    void deposit() {
        UI::clear();
        UI::title("  DEPOSIT  ");
        cout << "\n";
        int    acc = Input::getInt("  Account No : ");
        string pin = Input::getPin("  PIN        : ");

        Account* a = authenticate(acc, pin);
        if (!a) { UI::pause(); return; }

        double amt  = Input::getDouble("  Amount (INR): ");
        string note = Input::getString("  Remark      : ");
        if (note.empty()) note = "Cash deposit";

        a->deposit(amt, note);
        save();
        UI::success("Deposited INR " + to_string((int)amt) + " successfully.");
        cout << "  New Balance : INR " << fixed << setprecision(2) << a->getBalance() << "\n";
        UI::pause();
    }

    // ---- 3. Withdraw ------------------------------------------
    void withdraw() {
        UI::clear();
        UI::title("  WITHDRAWAL  ");
        cout << "\n";
        int    acc = Input::getInt("  Account No : ");
        string pin = Input::getPin("  PIN        : ");

        Account* a = authenticate(acc, pin);
        if (!a) { UI::pause(); return; }

        double amt  = Input::getDouble("  Amount (INR): ");
        string note = Input::getString("  Remark      : ");
        if (note.empty()) note = "Cash withdrawal";

        if (!a->withdraw(amt, note)) {
            UI::error("Insufficient funds. Minimum balance required: INR 500.");
        } else {
            save();
            UI::success("Withdrawn INR " + to_string((int)amt) + " successfully.");
            cout << "  New Balance : INR " << fixed << setprecision(2) << a->getBalance() << "\n";
        }
        UI::pause();
    }

    // ---- 4. Balance Check -------------------------------------
    void checkBalance() {
        UI::clear();
        UI::title("  BALANCE ENQUIRY  ");
        cout << "\n";
        int    acc = Input::getInt("  Account No : ");
        string pin = Input::getPin("  PIN        : ");

        Account* a = authenticate(acc, pin);
        if (!a) { UI::pause(); return; }

        cout << "\n";
        a->printSummary();
        UI::pause();
    }

    // ---- 5. Fund Transfer -------------------------------------
    void transfer() {
        UI::clear();
        UI::title("  FUND TRANSFER  ");
        cout << "\n";
        int    fromAcc = Input::getInt("  Your Account No  : ");
        string pin     = Input::getPin("  PIN              : ");

        Account* from = authenticate(fromAcc, pin);
        if (!from) { UI::pause(); return; }

        int toAcc = Input::getInt("  Beneficiary Acc  : ");
        if (toAcc == fromAcc) { UI::error("Cannot transfer to same account."); UI::pause(); return; }

        Account* to = find(toAcc);
        if (!to) { UI::error("Beneficiary account not found."); UI::pause(); return; }

        cout << "  Beneficiary: " << to->getName() << "\n";
        cout << "  Confirm transfer? (y/n): ";
        char c; cin >> c; cin.ignore();
        if (c != 'y' && c != 'Y') { UI::info("Transfer cancelled."); UI::pause(); return; }

        double amt = Input::getDouble("  Amount (INR)      : ");

        if (!from->debitTransfer(amt, toAcc)) {
            UI::error("Insufficient funds. Minimum balance required: INR 500.");
        } else {
            to->creditTransfer(amt, fromAcc);
            save();
            UI::success("INR " + to_string((int)amt) + " transferred to " + to->getName() + ".");
            cout << "  Your Balance: INR " << fixed << setprecision(2) << from->getBalance() << "\n";
        }
        UI::pause();
    }

    // ---- 6. Mini Statement ------------------------------------
    void miniStatement() {
        UI::clear();
        UI::title("  MINI STATEMENT  ");
        cout << "\n";
        int    acc = Input::getInt("  Account No : ");
        string pin = Input::getPin("  PIN        : ");

        Account* a = authenticate(acc, pin);
        if (!a) { UI::pause(); return; }

        cout << "\n";
        a->printStatement(10);
        UI::pause();
    }

    // ---- 7. Change PIN ----------------------------------------
    void changePin() {
        UI::clear();
        UI::title("  CHANGE PIN  ");
        cout << "\n";
        int    acc    = Input::getInt("  Account No  : ");
        string oldPin = Input::getPin("  Current PIN : ");

        Account* a = authenticate(acc, oldPin);
        if (!a) { UI::pause(); return; }

        string newPin1 = Input::getPin("  New PIN     : ");
        string newPin2 = Input::getPin("  Confirm     : ");
        if (newPin1 != newPin2) { UI::error("PINs do not match."); UI::pause(); return; }
        if (newPin1.size() < 4) { UI::error("PIN too short (min 4 digits)."); UI::pause(); return; }

        // Round-trip through serializer, patch pin line, rebuild
        string raw = a->serialize();
        istringstream reader(raw);
        string line;
        vector<string> lines;
        while (getline(reader, line)) lines.push_back(line);
        if (lines.size() >= 3) lines[2] = newPin1;
        ostringstream patched;
        for (auto& l : lines) patched << l << "\n";
        istringstream patchedStream(patched.str());
        Account rebuilt = Account::deserialize(patchedStream);

        for (auto& acc2 : accounts)
            if (acc2.getAccNo() == acc) { acc2 = rebuilt; break; }

        save();
        UI::success("PIN changed successfully.");
        UI::pause();
    }

    // ---- 8. List All Accounts (Admin) -------------------------
    void listAccounts() {
        UI::clear();
        UI::title("  ALL ACCOUNTS  ");
        if (accounts.empty()) { UI::info("No accounts found."); UI::pause(); return; }

        UI::border('-');
        cout << "  " << left
             << setw(10) << "Acc No"
             << setw(24) << "Holder"
             << setw(10) << "Type"
             << "Balance (INR)\n";
        UI::border('-');
        for (const auto& a : accounts) {
            cout << "  " << left
                 << setw(10) << a.getAccNo()
                 << setw(24) << a.getName()
                 << setw(10) << a.getType()
                 << fixed << setprecision(2) << a.getBalance() << "\n";
        }
        UI::border('-');
        cout << "  Total Accounts: " << accounts.size() << "\n";
        UI::pause();
    }

    const string& getName() const { return bankName; }
};

// ==============================================================
//  MAIN MENU
// ==============================================================
void showMenu(const string& bankName) {
    UI::clear();
    UI::title(" " + bankName + " ");
    cout << "\n"
         << "  -- Customer Services ----------------------------------\n"
         << "  [1]  Open New Account\n"
         << "  [2]  Deposit\n"
         << "  [3]  Withdraw\n"
         << "  [4]  Balance Enquiry\n"
         << "  [5]  Fund Transfer\n"
         << "  [6]  Mini Statement\n"
         << "  [7]  Change PIN\n"
         << "\n"
         << "  -- Admin ----------------------------------------------\n"
         << "  [8]  View All Accounts\n"
         << "\n"
         << "  [0]  Exit\n\n";
    UI::border('-');
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    Bank bank("NOVA BANK - Management System", "bank_data.txt");

    int choice;
    do {
        showMenu(bank.getName());
        choice = Input::getInt("  Enter choice: ");
        switch (choice) {
            case 1: bank.createAccount(); break;
            case 2: bank.deposit();       break;
            case 3: bank.withdraw();      break;
            case 4: bank.checkBalance();  break;
            case 5: bank.transfer();      break;
            case 6: bank.miniStatement(); break;
            case 7: bank.changePin();     break;
            case 8: bank.listAccounts();  break;
            case 0:
                UI::clear();
                UI::title("  THANK YOU FOR BANKING WITH US  ");
                cout << "\n  Records saved to: bank_data.txt\n\n";
                break;
            default:
                UI::error("Invalid choice. Please try again.");
                UI::pause();
        }
    } while (choice != 0);

    return 0;
}

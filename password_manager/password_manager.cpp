#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <codecvt>
#include <locale>

using namespace std;

struct PasswordRecord {
    wstring service;
    wstring login;
    wstring password;
};

// ===================== ID ЭЛЕМЕНТОВ ИНТЕРФЕЙСА =====================

#define ID_MASTER_EDIT      101
#define ID_OPEN_BUTTON      102

#define ID_SERVICE_EDIT     201
#define ID_LOGIN_EDIT       202
#define ID_LENGTH_EDIT      203
#define ID_PASSWORD_EDIT    204

#define ID_LOWER_CHECK      301
#define ID_UPPER_CHECK      302
#define ID_DIGIT_CHECK      303
#define ID_SPECIAL_CHECK    304

#define ID_GENERATE_BUTTON  401
#define ID_SAVE_BUTTON      402
#define ID_DELETE_BUTTON    403
#define ID_REFRESH_BUTTON   404

#define ID_LISTBOX          501
#define ID_STATUS_LABEL     601

// ===================== ГЛОБАЛЬНЫЕ HWND =====================

HWND hMasterEdit;
HWND hOpenButton;

HWND hServiceEdit;
HWND hLoginEdit;
HWND hLengthEdit;
HWND hPasswordEdit;

HWND hLowerCheck;
HWND hUpperCheck;
HWND hDigitCheck;
HWND hSpecialCheck;

HWND hGenerateButton;
HWND hSaveButton;
HWND hDeleteButton;
HWND hRefreshButton;

HWND hListBox;
HWND hStatusLabel;

vector<HWND> protectedControls;

// ===================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====================

wstring getWindowText(HWND hwnd) {
    int length = GetWindowTextLengthW(hwnd);

    if (length == 0) {
        return L"";
    }

    vector<wchar_t> buffer(length + 1);
    GetWindowTextW(hwnd, buffer.data(), length + 1);

    return wstring(buffer.data());
}

void setWindowText(HWND hwnd, const wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

void showMessage(const wstring& text) {
    MessageBoxW(nullptr, text.c_str(), L"Сообщение", MB_OK | MB_ICONINFORMATION);
}

void showError(const wstring& text) {
    MessageBoxW(nullptr, text.c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
}

bool isChecked(HWND hwnd) {
    return SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void setControlsEnabled(bool enabled) {
    for (HWND control : protectedControls) {
        EnableWindow(control, enabled);
    }
}

string wstringToUtf8(const wstring& str)
{
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(str);
}

wstring utf8ToWstring(const string& str)
{
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// ===================== ГЕНЕРАТОР ПАРОЛЕЙ =====================

class PasswordGenerator {
private:
    wstring lowerChars = L"abcdefghijklmnopqrstuvwxyz";
    wstring upperChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    wstring digitChars = L"0123456789";
    wstring specialChars = L"!@#$%^&*()-_=+[]{};:,.<>?/";

    wchar_t getRandomChar(const wstring& chars, mt19937& generator) {
        uniform_int_distribution<int> dist(0, static_cast<int>(chars.size()) - 1);
        return chars[dist(generator)];
    }

public:
    wstring generatePassword(int length, bool useLower, bool useUpper, bool useDigits, bool useSpecial) {
        vector<wstring> groups;
        wstring allChars;

        if (useLower) {
            groups.push_back(lowerChars);
            allChars += lowerChars;
        }

        if (useUpper) {
            groups.push_back(upperChars);
            allChars += upperChars;
        }

        if (useDigits) {
            groups.push_back(digitChars);
            allChars += digitChars;
        }

        if (useSpecial) {
            groups.push_back(specialChars);
            allChars += specialChars;
        }

        random_device rd;
        mt19937 randomGenerator(rd());

        wstring password;

        for (const wstring& group : groups) {
            password += getRandomChar(group, randomGenerator);
        }

        while (static_cast<int>(password.size()) < length) {
            password += getRandomChar(allChars, randomGenerator);
        }

        shuffle(password.begin(), password.end(), randomGenerator);

        return password;
    }
};

// ===================== ХРАНИЛИЩЕ ПАРОЛЕЙ =====================

class PasswordStorage {
private:
    string filename;
    wstring masterPassword;
    vector<PasswordRecord> records;

public:
    PasswordStorage(const string& filename)
        : filename(filename) {}

    bool fileExists() const {
        ifstream file(filename);
        return file.good() && file.peek() != ifstream::traits_type::eof();
    }

    bool createVault(const wstring& master) {
        masterPassword = master;
        records.clear();
        return save();
    }

    bool openVault(const wstring& master) {
        ifstream file(filename);

        if (!file.is_open()) {
            return false;
        }

        string header;
        getline(file, header);

        if (header != "PASSWORD_MANAGER_GUI_V1") {
            return false;
        }

        string masterLine;
        getline(file, masterLine);

        string prefix = "MASTER_PASSWORD=";

        if (masterLine.find(prefix) != 0) {
            return false;
        }

        string savedPassword =
            masterLine.substr(prefix.size());

        string enteredPassword(
            master.begin(),
            master.end()
        );

        if (savedPassword != enteredPassword) {
            return false;
        }

        masterPassword = master;
        records.clear();

        string line;

        while (getline(file, line)) {

            if (line.find("SERVICE=") != 0) {
                continue;
            }

            PasswordRecord record;

            string service =
                line.substr(8);

            if (!getline(file, line)) {
                break;
            }

            if (line.find("LOGIN=") != 0) {
                break;
            }

            string login =
                line.substr(6);

            if (!getline(file, line)) {
                break;
            }

            if (line.find("PASSWORD=") != 0) {
                break;
            }

            string password =
                line.substr(9);

            record.service = utf8ToWstring(service);
            record.login = utf8ToWstring(login);
            record.password = utf8ToWstring(password);

            records.push_back(record);
        }

        return true;
    }

    bool save() const {
        ofstream file(filename, ios::trunc);

        if (!file.is_open()) {
            return false;
        }

        file << "PASSWORD_MANAGER_GUI_V1\n";

        string master(
            masterPassword.begin(),
            masterPassword.end()
        );

        file << "MASTER_PASSWORD="
            << master
            << "\n";

        for (const auto& record : records) {

            string service = wstringToUtf8(record.service);
            string login = wstringToUtf8(record.login);
            string password = wstringToUtf8(record.password);

            file << "SERVICE="
                << service
                << "\n";

            file << "LOGIN="
                << login
                << "\n";

            file << "PASSWORD="
                << password
                << "\n";
        }

        return true;
    }

    void addRecord(const PasswordRecord& record) {
        records.push_back(record);
        save();
    }

    bool removeRecord(int index) {

        if (index < 0 ||
            index >= static_cast<int>(records.size())) {
            return false;
        }

        records.erase(records.begin() + index);

        return save();
    }

    vector<PasswordRecord>& getRecords() {
        return records;
    }
};

// ===================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ =====================

PasswordGenerator passwordGenerator;
PasswordStorage storage("../vault_gui.txt");
bool vaultOpened = false;

// ===================== ОБНОВЛЕНИЕ СПИСКА =====================

void updateListBox() {
    SendMessageW(hListBox, LB_RESETCONTENT, 0, 0);

    vector<PasswordRecord>& records = storage.getRecords();

    for (size_t i = 0; i < records.size(); i++) {
        wstring line = to_wstring(i + 1) + L". "
            + records[i].service + L" | "
            + records[i].login + L" | "
            + records[i].password;

        SendMessageW(hListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }
}

// ===================== ОСНОВНЫЕ ДЕЙСТВИЯ =====================

void openOrCreateVault() {
    wstring master = getWindowText(hMasterEdit);

    if (master.empty()) {
        showError(L"Введите мастер-пароль.");
        return;
    }

    if (!storage.fileExists()) {
        if (storage.createVault(master)) {
            vaultOpened = true;
            setControlsEnabled(true);
            setWindowText(hStatusLabel, L"Хранилище создано и открыто.");
            showMessage(L"Хранилище создано.");
        }
        else {
            showError(L"Не удалось создать файл хранилища.");
        }
    }
    else {
        if (storage.openVault(master)) {
            vaultOpened = true;
            setControlsEnabled(true);
            updateListBox();
            setWindowText(hStatusLabel, L"Хранилище открыто.");
            showMessage(L"Хранилище успешно открыто.");
        }
        else {
            showError(L"Неверный мастер-пароль.");
        }
    }
}

void generatePasswordButtonClick() {
    if (!vaultOpened) {
        showError(L"Сначала откройте хранилище.");
        return;
    }

    wstring lengthText = getWindowText(hLengthEdit);
    int length = _wtoi(lengthText.c_str());

    if (length < 8 || length > 128) {
        showError(L"Длина пароля должна быть от 8 до 128.");
        return;
    }

    bool useLower = isChecked(hLowerCheck);
    bool useUpper = isChecked(hUpperCheck);
    bool useDigits = isChecked(hDigitCheck);
    bool useSpecial = isChecked(hSpecialCheck);

    if (!useLower && !useUpper && !useDigits && !useSpecial) {
        showError(L"Выберите хотя бы одну группу символов.");
        return;
    }

    wstring password = passwordGenerator.generatePassword(
        length,
        useLower,
        useUpper,
        useDigits,
        useSpecial
    );

    setWindowText(hPasswordEdit, password);
}

void savePasswordButtonClick() {
    if (!vaultOpened) {
        showError(L"Сначала откройте хранилище.");
        return;
    }

    PasswordRecord record;

    record.service = getWindowText(hServiceEdit);
    record.login = getWindowText(hLoginEdit);
    record.password = getWindowText(hPasswordEdit);

    if (record.service.empty()) {
        showError(L"Введите название сервиса.");
        return;
    }

    if (record.password.empty()) {
        showError(L"Сначала сгенерируйте или введите пароль.");
        return;
    }

    storage.addRecord(record);
    updateListBox();

    setWindowText(hServiceEdit, L"");
    setWindowText(hLoginEdit, L"");
    setWindowText(hPasswordEdit, L"");

    showMessage(L"Пароль сохранён.");
}

void deletePasswordButtonClick() {
    if (!vaultOpened) {
        showError(L"Сначала откройте хранилище.");
        return;
    }

    int selectedIndex = static_cast<int>(SendMessageW(hListBox, LB_GETCURSEL, 0, 0));

    if (selectedIndex == LB_ERR) {
        showError(L"Выберите запись для удаления.");
        return;
    }

    int result = MessageBoxW(
        nullptr,
        L"Вы точно хотите удалить выбранную запись?",
        L"Подтверждение",
        MB_YESNO | MB_ICONQUESTION
    );

    if (result == IDYES) {
        if (storage.removeRecord(selectedIndex)) {
            updateListBox();
            showMessage(L"Запись удалена.");
        }
        else {
            showError(L"Ошибка удаления.");
        }
    }
}

// ===================== СОЗДАНИЕ ИНТЕРФЕЙСА =====================

void createInterface(HWND hwnd) {
    HFONT font = CreateFontW(
        18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    auto createLabel = [&](const wchar_t* text, int x, int y, int w, int h) -> HWND {
        HWND control = CreateWindowW(
            L"STATIC",
            text,
            WS_VISIBLE | WS_CHILD,
            x,
            y,
            w,
            h,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );

        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return control;
        };

    auto createEdit = [&](int id, int x, int y, int w, int h, bool password = false) -> HWND {
        DWORD style = WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL;

        if (password) {
            style |= ES_PASSWORD;
        }

        HWND control = CreateWindowW(
            L"EDIT",
            L"",
            style,
            x,
            y,
            w,
            h,
            hwnd,
            (HMENU)(INT_PTR)id,
            nullptr,
            nullptr
        );

        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return control;
        };

    auto createButton = [&](const wchar_t* text, int id, int x, int y, int w, int h) -> HWND {
        HWND control = CreateWindowW(
            L"BUTTON",
            text,
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x,
            y,
            w,
            h,
            hwnd,
            (HMENU)(INT_PTR)id,
            nullptr,
            nullptr
        );

        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return control;
        };

    auto createCheckBox = [&](const wchar_t* text, int id, int x, int y, int w, int h, bool checked) -> HWND {
        HWND control = CreateWindowW(
            L"BUTTON",
            text,
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            x,
            y,
            w,
            h,
            hwnd,
            (HMENU)(INT_PTR)id,
            nullptr,
            nullptr
        );

        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

        if (checked) {
            SendMessageW(control, BM_SETCHECK, BST_CHECKED, 0);
        }

        return control;
        };

    createLabel(L"Мастер-пароль:", 20, 20, 150, 25);
    hMasterEdit = createEdit(ID_MASTER_EDIT, 180, 20, 250, 25, true);
    hOpenButton = createButton(L"Открыть / создать", ID_OPEN_BUTTON, 450, 20, 180, 30);

    hStatusLabel = createLabel(L"Введите мастер-пароль для доступа к хранилищу.", 20, 60, 650, 25);

    createLabel(L"Сервис:", 20, 110, 120, 25);
    hServiceEdit = createEdit(ID_SERVICE_EDIT, 180, 110, 250, 25);

    createLabel(L"Логин / почта:", 20, 150, 150, 25);
    hLoginEdit = createEdit(ID_LOGIN_EDIT, 180, 150, 250, 25);

    createLabel(L"Длина пароля:", 20, 190, 150, 25);
    hLengthEdit = createEdit(ID_LENGTH_EDIT, 180, 190, 80, 25);
    setWindowText(hLengthEdit, L"12");

    hLowerCheck = createCheckBox(L"Маленькие буквы", ID_LOWER_CHECK, 20, 235, 180, 25, true);
    hUpperCheck = createCheckBox(L"Большие буквы", ID_UPPER_CHECK, 220, 235, 180, 25, true);
    hDigitCheck = createCheckBox(L"Цифры", ID_DIGIT_CHECK, 420, 235, 100, 25, true);
    hSpecialCheck = createCheckBox(L"Спец. символы", ID_SPECIAL_CHECK, 540, 235, 160, 25, true);

    createLabel(L"Пароль:", 20, 280, 120, 25);
    hPasswordEdit = createEdit(ID_PASSWORD_EDIT, 180, 280, 350, 25);

    hGenerateButton = createButton(L"Сгенерировать", ID_GENERATE_BUTTON, 550, 275, 160, 35);
    hSaveButton = createButton(L"Сохранить", ID_SAVE_BUTTON, 550, 320, 160, 35);

    createLabel(L"Сохранённые пароли:", 20, 340, 250, 25);

    hListBox = CreateWindowW(
        L"LISTBOX",
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        20,
        370,
        690,
        180,
        hwnd,
        (HMENU)(INT_PTR)ID_LISTBOX,
        nullptr,
        nullptr
    );

    SendMessageW(hListBox, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    hDeleteButton = createButton(L"Удалить выбранное", ID_DELETE_BUTTON, 20, 565, 180, 35);
    hRefreshButton = createButton(L"Обновить список", ID_REFRESH_BUTTON, 220, 565, 180, 35);

    protectedControls = {
        hServiceEdit,
        hLoginEdit,
        hLengthEdit,
        hPasswordEdit,
        hLowerCheck,
        hUpperCheck,
        hDigitCheck,
        hSpecialCheck,
        hGenerateButton,
        hSaveButton,
        hDeleteButton,
        hRefreshButton,
        hListBox
    };

    setControlsEnabled(false);
}

// ===================== ОБРАБОТЧИК ОКНА =====================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        createInterface(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_OPEN_BUTTON:
            openOrCreateVault();
            break;

        case ID_GENERATE_BUTTON:
            generatePasswordButtonClick();
            break;

        case ID_SAVE_BUTTON:
            savePasswordButtonClick();
            break;

        case ID_DELETE_BUTTON:
            deletePasswordButtonClick();
            break;

        case ID_REFRESH_BUTTON:
            updateListBox();
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    const wchar_t CLASS_NAME[] = L"PasswordManagerWindow";

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Менеджер паролей",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        760,
        670,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
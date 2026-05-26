from flask import Flask, render_template
import re
import os

app = Flask(__name__)

VAULT_FILE = "vault_gui.txt"


def password_strength(password):
    score = 0

    length_score = len(password) // 4
    score += min(length_score, 4) 
    
    lower_chars = r"[abcdefghijklmnopqrstuvwxyz]"
    upper_chars = r"[ABCDEFGHIJKLMNOPQRSTUVWXYZ]"
    digit_chars = r"[0123456789]"
    special_chars = r"[!@#\$%\^&\*\(\)\-_\+=\[\]\{\};:,\.<>\?\/]"
    
    if re.search(lower_chars, password):
        score += 1
    if re.search(upper_chars, password):
        score += 1
    if re.search(digit_chars, password):
        score += 1
    if re.search(special_chars, password):
        score += 1

    if score <= 3:
        return "Очень слабый"
    elif score <= 5:
        return "Слабый"
    elif score <= 7:
        return "Средний"
    else:
        return "Сильный"


def read_passwords():
    services = []
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, ".."))

    FILE_PATH = os.path.join(PROJECT_ROOT, "password_manager", "vault_gui.txt")

    with open(FILE_PATH, "r", encoding="utf-8") as f:
        data = f.read()

    lines = data.splitlines()

    i = 0

    while i < len(lines):

        if lines[i].startswith("SERVICE="):

            service = lines[i][8:]

            login = ""
            password = ""

            if i + 1 < len(lines) and lines[i + 1].startswith("LOGIN="):
                login = lines[i + 1][6:]

            if i + 2 < len(lines) and lines[i + 2].startswith("PASSWORD="):
                password = lines[i + 2][9:]

            services.append({
                "service": service,
                "login": login,
                "password": password,
                "strength": password_strength(password)
            })

            i += 3
        else:
            i += 1

    return services


@app.route('/')
def hello_world():
    passwords = read_passwords()
    return render_template(
        'check.html',
        passwords=passwords
    )


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5002)
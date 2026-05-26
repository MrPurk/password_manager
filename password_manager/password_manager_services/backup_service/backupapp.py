from flask import Flask
import shutil
import os
from datetime import datetime

app = Flask(__name__)

VAULT_FILE = "/password_manager/vault_gui.txt"
BACKUP_DIR = "/password_manager/backups"


def create_backup():

    if not os.path.exists(VAULT_FILE):
        return "Vault file not found"

    os.makedirs(BACKUP_DIR, exist_ok=True)

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    backup_file = os.path.join(
        BACKUP_DIR,
        f"vault_backup_{timestamp}.txt"
    )

    shutil.copy2(VAULT_FILE, backup_file)

    return backup_file


@app.route('/')
def home():

    backup_file = create_backup()

    return f"""
    <h1>Backup created</h1>
    <p>{backup_file}</p>
    """


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)
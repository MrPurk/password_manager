FROM python:3.10-slim
WORKDIR /app
RUN pip install flask
COPY app.py .
EXPOSE 5001
CMD ["python", "backupapp.py"]
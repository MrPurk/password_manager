FROM python:3.10-slim
WORKDIR /app
RUN pip install flask
COPY app.py .
EXPOSE 5002
CMD ["python", "checkerapp.py"]
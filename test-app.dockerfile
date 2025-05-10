# Use the official Python image as the base
FROM python:3.11-slim

# Set the working directory in the container
WORKDIR /app

# Copy the app into the container
COPY app.py .

# Set the command to run the program
CMD ["python", "app.py"]

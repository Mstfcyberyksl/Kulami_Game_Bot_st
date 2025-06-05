# Cloud Upload Setup for Kulami Game Bot

This guide explains how to set up automatic file uploads when your Kulami game simulation finishes.

## Quick Setup

1. Run the setup script:
   ```bash
   ./setup_cloud_upload.sh
   ```

2. Choose and configure your preferred upload method by editing `upload_config.py`

## Upload Options

### 1. Google Drive (Recommended)
- **Pros**: Large free storage (15GB), reliable, widely used
- **Setup**:
  1. Go to [Google Cloud Console](https://console.cloud.google.com/)
  2. Create a new project or select existing one
  3. Enable Google Drive API
  4. Create OAuth 2.0 credentials and download as `credentials.json`
  5. Place `credentials.json` in this directory
  6. Set `USE_GOOGLE_DRIVE = True` in `upload_config.py`

### 2. Dropbox
- **Pros**: Easy API setup, good for smaller files
- **Setup**:
  1. Go to [Dropbox Developers](https://www.dropbox.com/developers/apps)
  2. Create a new app
  3. Generate access token
  4. Edit `upload_config.py` and set:
     ```python
     USE_DROPBOX = True
     DROPBOX_TOKEN = "your_access_token_here"
     ```

### 3. Email Upload
- **Pros**: Simple, works anywhere, no API keys needed
- **Setup**:
  1. Edit `upload_config.py` and configure:
     ```python
     USE_EMAIL = True
     EMAIL_FROM = "your_email@gmail.com"
     EMAIL_PASSWORD = "your_app_password"  # For Gmail, use app password
     EMAIL_TO = "destination@email.com"
     ```

### 4. HTTP Server Upload
- **Pros**: Full control, can use any server
- **Setup**:
  1. Set up a server that accepts file uploads
  2. Edit `upload_config.py`:
     ```python
     USE_HTTP_UPLOAD = True
     HTTP_UPLOAD_URL = "https://your-server.com/upload"
     ```

## Running the Program

After setup, run your simulation as usual:

```bash
# Activate virtual environment
source venv/bin/activate

# Run the simulation
python generate.py
```

The files will be automatically uploaded when:
- The simulation finishes normally
- You press Ctrl+C to interrupt

## Files That Get Uploaded

- `data.csv` - Main simulation data
- `array_backup.pkl` - Backup of the current state

Files are automatically timestamped to avoid conflicts.

## Troubleshooting

- **Import errors**: Run `./setup_cloud_upload.sh` to install dependencies
- **Google Drive 403 error**: Make sure you've enabled the Drive API in Google Cloud Console
- **Email authentication error**: Use an app password, not your regular password
- **No uploads successful**: Check your configuration in `upload_config.py`

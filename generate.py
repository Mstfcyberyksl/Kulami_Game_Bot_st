import ctypes
import signal
import pickle
import time
import os
from datetime import datetime

# Google Drive upload imports (install with: pip install google-api-python-client google-auth-httplib2 google-auth-oauthlib)
try:
    from googleapiclient.discovery import build
    from googleapiclient.http import MediaFileUpload
    from google.auth.transport.requests import Request
    from google.oauth2.credentials import Credentials
    from google_auth_oauthlib.flow import InstalledAppFlow
    GOOGLE_DRIVE_AVAILABLE = True
except ImportError:
    print("Google Drive libraries not installed. Install with: pip install google-api-python-client google-auth-httplib2 google-auth-oauthlib")
    GOOGLE_DRIVE_AVAILABLE = False

# Load upload configuration
try:
    from upload_config import *
except ImportError:
    print("upload_config.py not found, using defaults")
    USE_GOOGLE_DRIVE = True
    USE_HTTP_UPLOAD = False

# If modifying these scopes, delete the file token.json.
SCOPES = ['https://www.googleapis.com/auth/drive.file']

def upload_to_google_drive(file_path, file_name=None):
    """Upload a file to Google Drive"""
    if not GOOGLE_DRIVE_AVAILABLE:
        print("Google Drive upload skipped - libraries not available")
        return False
    
    if not file_name:
        file_name = os.path.basename(file_path)
    
    # Add timestamp to filename
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    name, ext = os.path.splitext(file_name)
    file_name = f"{name}_{timestamp}{ext}"
    
    try:
        creds = None
        # The file token.json stores the user's access and refresh tokens.
        if os.path.exists('token.json'):
            creds = Credentials.from_authorized_user_file('token.json', SCOPES)
        
        # If there are no (valid) credentials available, let the user log in.
        if not creds or not creds.valid:
            if creds and creds.expired and creds.refresh_token:
                creds.refresh(Request())
            else:
                if not os.path.exists('credentials.json'):
                    print("Google Drive credentials.json not found. Please download it from Google Cloud Console.")
                    return False
                flow = InstalledAppFlow.from_client_secrets_file('credentials.json', SCOPES)
                creds = flow.run_local_server(port=0)
            # Save the credentials for the next run
            with open('token.json', 'w') as token:
                token.write(creds.to_json())

        service = build('drive', 'v3', credentials=creds)
        
        # Upload file
        media = MediaFileUpload(file_path, resumable=True)
        file_metadata = {'name': file_name}
        
        file = service.files().create(body=file_metadata, media_body=media, fields='id').execute()
        print(f'File uploaded to Google Drive successfully! File ID: {file.get("id")}')
        print(f'File name: {file_name}')
        return True
        
    except Exception as e:
        print(f'Error uploading to Google Drive: {e}')
        return False

# Alternative: Simple HTTP upload to any server
def upload_to_http_server(file_path, upload_url, file_name=None):
    """Upload file to any HTTP server that accepts file uploads"""
    try:
        import requests
        
        if not file_name:
            file_name = os.path.basename(file_path)
        
        # Add timestamp to filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        name, ext = os.path.splitext(file_name)
        file_name = f"{name}_{timestamp}{ext}"
        
        with open(file_path, 'rb') as f:
            files = {'file': (file_name, f, 'application/octet-stream')}
            response = requests.post(upload_url, files=files, timeout=30)
        
        if response.status_code == 200:
            print(f'File uploaded to server successfully!')
            print(f'Response: {response.text}')
            return True
        else:
            print(f'Upload failed with status code: {response.status_code}')
            return False
            
    except Exception as e:
        print(f'Error uploading to HTTP server: {e}')
        return False

def upload_files_to_cloud():
    """Upload data.csv and array_backup.pkl to cloud storage"""
    files_to_upload = []
    
    # Check for data.csv
    if os.path.exists('data.csv'):
        files_to_upload.append('data.csv')
    
    # Check for array_backup.pkl
    if os.path.exists('array_backup.pkl'):
        files_to_upload.append('array_backup.pkl')
    
    if not files_to_upload:
        print("No files found to upload")
        return
    
    print(f"\nUploading {len(files_to_upload)} files to cloud storage...")
    upload_success = False
    
    # Try Google Drive
    if USE_GOOGLE_DRIVE:
        print("\nTrying Google Drive upload...")
        for file_path in files_to_upload:
            print(f"Uploading {file_path} to Google Drive...")
            success = upload_to_google_drive(file_path)
            if success:
                upload_success = True
                print(f"✓ {file_path} uploaded to Google Drive successfully")
            else:
                print(f"✗ Failed to upload {file_path} to Google Drive")
    
    
    # Try HTTP upload
    if USE_HTTP_UPLOAD and HTTP_UPLOAD_URL != "https://your-server.com/upload":
        print("\nTrying HTTP server upload...")
        for file_path in files_to_upload:
            print(f"Uploading {file_path} to HTTP server...")
            success = upload_to_http_server(file_path, HTTP_UPLOAD_URL)
            if success:
                upload_success = True
                print(f"✓ {file_path} uploaded to HTTP server successfully")
            else:
                print(f"✗ Failed to upload {file_path} to HTTP server")
    
    
    if not upload_success:
        print("\n⚠️  No uploads were successful. Files remain local:")
        for file_path in files_to_upload:
            print(f"  - {file_path}")
        print("\nPlease check your configuration in upload_config.py")


functionbest = ctypes.CDLL("./kulami_game.so")
functionbest.best_place.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int)
functionbest.best_place.restype = ctypes.POINTER(ctypes.c_int)
functionbest.main()

# ilk 64 tanesi board position sonrası x y step lx ly userframe pcframe
array = [[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4, 4, 17, -1, -1, -1, -1]]
directions = [(7,0),(6,0),(5,0),(4,0),(3,0),(2,0),(1,0),(-7,0),(-6,0),(-5,0),(-4,0),(-3,0),(-2,0),(-1,0),(0,1),(0,2),(0,3),(0,4),(0,5),(0,6),(0,7),(0,-1),(0,-2),(0,-3),(0,-4),(0,-5),(0,-6),(0,-7)]
frames = [[4,0,0,0,1,1,0,1,1,-1,-1,-1,-1],
          [6,0,2,0,3,0,4,1,2,1,3,1,4],
          [6,0,5,0,6,1,5,1,6,2,5,2,6],
          [3,0,7,1,7,2,7,-1,-1,-1,-1,-1,-1],
          [2,2,0,3,0,-1,-1,-1,-1,-1,-1,-1,-1],
          [4,2,1,2,2,3,1,3,2,-1,-1,-1,-1],
          [2,2,3,2,4,-1,-1,-1,-1,-1,-1,-1,-1],
          [3,4,0,4,1,4,2,-1,-1,-1,-1,-1,-1],
          [6,3,3,3,4,3,5,4,3,4,4,4,5],
          [2,3,6,3,7,-1,-1,-1,-1,-1,-1,-1,-1],
          [4,4,6,4,7,5,6,5,7,-1,-1,-1,-1],
          [6,5,0,5,1,6,0,6,1,7,0,7,1],
          [2,5,2,6,2,-1,-1,-1,-1,-1,-1,-1,-1],
          [4,5,3,5,4,6,3,6,4,-1,-1,-1,-1],
          [3,5,5,6,5,7,5,-1,-1,-1,-1,-1,-1],
          [3,7,2,7,3,7,4,-1,-1,-1,-1,-1,-1],
          [4,6,6,6,7,7,6,7,7,-1,-1,-1,-1]]
def find_frame(array):
	for i in range(len(frames)):
		j = 1
		while j < 2 * frames[i][0]:
			if frames[i][j] == array[0] and frames[i][j + 1] == array[1]:
				return i
			j += 2

	return -1

def piece_count(data):
	count = 0
	for i in range(64):
		if data[i] != 0:
			count += 1
	return count
def is_valid(data,x,y,result):
    if result[0] + x < 8 and result[0] + x >= 0 and result[1] + y < 8 and result[1] + y >= 0 and data[8*(result[0] + x) + (result[1] + y)] == 0 and find_frame([result[0] + x, result[1] + y]) != data[-2] and find_frame([result[0] + x, result[1] + y]) != data[-1] and piece_count(data) < 56:
        return True
    return False

def add_data(data,result):
    data[-2] = find_frame([data[-7],data[-6]])    
    data[-4] = result[0]
    data[-3] = result[1]
    for direction in directions:
        x = direction[0]
        y = direction[1]
        if is_valid(data,x,y,result):
            data[-7] = result[0] + x
            data[-6] = result[1] + y
            array.append(data.copy())
			
times = 0
def signal_handler(_sig, _frame):
    print('\nCtrl+C detected! Saving array to file...')
    with open('array_backup.pkl', 'wb') as f:
        pickle.dump(array, f)
    print(f'Array saved to array_backup.pkl with {len(array)} elements')
    
    # Upload files to cloud before exiting
    #upload_files_to_cloud()
    exit(0)

signal.signal(signal.SIGINT, signal_handler)

while len(array) > 0:
    print("Data : ")
    print(array)
    times += 1
    if times % 10 == 0:
        print("Times: ", times, "Array length: ", len(array))
    data = array.pop()
    start_time = time.time()
    result = functionbest.best_place(data[-7], data[-6], data[-5], data[-4],data[-3],data[-2],data[-1])
    end_time = time.time()
    execution_time = end_time - start_time
    print(f"Function execution time: {execution_time:.6f} seconds")
    data[8*data[-7] + data[-6]] = 1
    data[8*result[0] + result[1]] = 2
    data[-1] = find_frame(result)
    add_data(data,result)

# When the main loop finishes, upload files to cloud
print("\nProcessing completed! Uploading files to cloud storage...")
#upload_files_to_cloud()
print("Program finished.")



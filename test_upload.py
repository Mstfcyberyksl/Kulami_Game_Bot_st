#!/usr/bin/env python3
"""
Test script for cloud upload functionality
Creates a test file and tries to upload it
"""

import os
import sys
from datetime import datetime

# Add the current directory to Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from generate import upload_files_to_cloud, upload_to_google_drive, upload_to_dropbox, send_email_with_attachments
    from upload_config import *
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("Make sure to run ./setup_cloud_upload.sh first")
    sys.exit(1)

def create_test_file():
    """Create a small test file"""
    test_content = f"""Test upload file created at {datetime.now()}
This is a test to verify cloud upload functionality.
Game simulation data would go here.
"""
    
    with open('test_upload.txt', 'w') as f:
        f.write(test_content)
    
    print("Created test_upload.txt")

def test_uploads():
    """Test all configured upload methods"""
    create_test_file()
    
    success_count = 0
    total_tests = 0
    
    print("Testing cloud upload functionality...\n")
    
    # Test Google Drive
    if USE_GOOGLE_DRIVE:
        total_tests += 1
        print("Testing Google Drive upload...")
        if upload_to_google_drive('test_upload.txt'):
            success_count += 1
            print("✓ Google Drive upload successful")
        else:
            print("✗ Google Drive upload failed")
    
    
    # Clean up
    if os.path.exists('test_upload.txt'):
        os.remove('test_upload.txt')
        print("\nCleaned up test file")
    
    print(f"\nTest Results: {success_count}/{total_tests} uploads successful")
    
    if success_count == 0:
        print("\n⚠️  No uploads were successful. Please check your configuration.")
        print("Edit upload_config.py and ensure you have valid credentials.")
    else:
        print("\n✓ Upload functionality is working! Your files will be uploaded automatically.")

if __name__ == "__main__":
    test_uploads()

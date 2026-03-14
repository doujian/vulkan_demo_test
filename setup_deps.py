#!/usr/bin/env python3
"""
Setup script to download third-party dependencies.
Run this before building the project.
"""

import os
import sys
import zipfile
import shutil
import time
import urllib.request

THIRD_PARTY_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "third_party")
MAX_RETRIES = 3
RETRY_DELAY = 2
CHUNK_SIZE = 1024 * 1024

DEPENDENCIES = {
    "glm": {
        "url": "https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.zip",
        "type": "zip",
        "strip_components": 1,
    },
    "tinygltf": {
        "files": [
            ("https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h", "tiny_gltf.h"),
            ("https://raw.githubusercontent.com/syoyo/tinygltf/release/json.hpp", "json.hpp"),
        ]
    },
    "stb": {
        "files": [
            ("https://raw.githubusercontent.com/nothings/stb/master/stb_image.h", "stb_image.h"),
            ("https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h", "stb_image_write.h"),
        ]
    },
}


def format_size(size):
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024:
            return f"{size:.1f}{unit}"
        size /= 1024
    return f"{size:.1f}TB"


def download_with_progress(url, dest_path):
    for attempt in range(MAX_RETRIES):
        try:
            print(f"  Downloading {url}... (attempt {attempt + 1}/{MAX_RETRIES})")
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            
            request = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            response = urllib.request.urlopen(request, timeout=60)
            total_size = int(response.headers.get('content-length', 0))
            
            downloaded = 0
            start_time = time.time()
            
            with open(dest_path, 'wb') as f:
                while True:
                    chunk = response.read(CHUNK_SIZE)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        percent = downloaded * 100 / total_size
                        speed = downloaded / (time.time() - start_time + 0.001)
                        print(f"\r  Progress: {percent:.1f}% ({format_size(downloaded)}/{format_size(total_size)}) {format_size(speed)}/s", end='', flush=True)
            
            print()
            return
        except Exception as e:
            if attempt < MAX_RETRIES - 1:
                print(f"\n  Failed: {e}, retrying in {RETRY_DELAY}s...")
                time.sleep(RETRY_DELAY)
            else:
                raise


def download_file(url, dest_path):
    download_with_progress(url, dest_path)


def download_zip(url, dest_dir, strip_components=0):
    os.makedirs(dest_dir, exist_ok=True)
    temp_path = os.path.join(dest_dir, "temp.zip")
    
    download_with_progress(url, temp_path)
    
    with zipfile.ZipFile(temp_path, 'r') as zip_ref:
        if strip_components == 0:
            zip_ref.extractall(dest_dir)
        else:
            for member in zip_ref.namelist():
                parts = member.split('/')
                if len(parts) <= strip_components:
                    continue
                new_path = '/'.join(parts[strip_components:])
                if not new_path:
                    continue
                target = os.path.join(dest_dir, new_path)
                if member.endswith('/'):
                    os.makedirs(target, exist_ok=True)
                else:
                    os.makedirs(os.path.dirname(target), exist_ok=True)
                    with zip_ref.open(member) as source, open(target, 'wb') as f:
                        f.write(source.read())
    
    os.remove(temp_path)


def setup_glm():
    dest_dir = os.path.join(THIRD_PARTY_DIR, "glm")
    if os.path.exists(os.path.join(dest_dir, "glm")):
        print("glm already exists, skipping...")
        return
    
    print("Setting up glm...")
    os.makedirs(dest_dir, exist_ok=True)
    
    dep = DEPENDENCIES["glm"]
    if "url" in dep:
        download_zip(dep["url"], dest_dir, dep.get("strip_components", 0))
    print("  Done.")


def setup_tinygltf():
    dest_dir = os.path.join(THIRD_PARTY_DIR, "tinygltf")
    if os.path.exists(os.path.join(dest_dir, "tiny_gltf.h")):
        print("tinygltf already exists, skipping...")
        return
    
    print("Setting up tinygltf...")
    os.makedirs(dest_dir, exist_ok=True)
    
    dep = DEPENDENCIES["tinygltf"]
    for url, filename in dep["files"]:
        download_file(url, os.path.join(dest_dir, filename))
    print("  Done.")


def setup_stb():
    dest_dir = os.path.join(THIRD_PARTY_DIR, "stb")
    if os.path.exists(os.path.join(dest_dir, "stb_image.h")):
        print("stb already exists, skipping...")
        return
    
    print("Setting up stb...")
    os.makedirs(dest_dir, exist_ok=True)
    
    dep = DEPENDENCIES["stb"]
    for url, filename in dep["files"]:
        dest_path = os.path.join(dest_dir, filename)
        if not os.path.exists(dest_path):
            download_file(url, dest_path)
    print("  Done.")


def setup_ktx():
    dest_dir = os.path.join(THIRD_PARTY_DIR, "ktx")
    if os.path.exists(os.path.join(dest_dir, "lib")):
        print("ktx already exists, skipping...")
        return
    
    if os.path.exists(dest_dir):
        print("Removing incomplete ktx directory...")
        shutil.rmtree(dest_dir, ignore_errors=True)
    
    print("Setting up ktx...")
    url = "https://github.com/KhronosGroup/KTX-Software/archive/refs/tags/v4.4.2.zip"
    download_zip(url, dest_dir, strip_components=1)
    print("  Done.")


def main():
    print("=" * 50)
    print("Setting up third-party dependencies")
    print("=" * 50)
    
    os.makedirs(THIRD_PARTY_DIR, exist_ok=True)
    
    setup_glm()
    setup_tinygltf()
    setup_stb()
    setup_ktx()
    
    print("=" * 50)
    print("All dependencies installed successfully!")
    print("=" * 50)


if __name__ == "__main__":
    main()
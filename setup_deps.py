#!/usr/bin/env python3
"""
Setup script to download third-party dependencies.
Run this before building the project.
"""

import os
import sys
import urllib.request
import zipfile
import shutil

THIRD_PARTY_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "third_party")

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


def download_file(url, dest_path):
    print(f"  Downloading {url}...")
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    urllib.request.urlretrieve(url, dest_path)


def download_zip(url, dest_dir, strip_components=0):
    print(f"  Downloading {url}...")
    temp_path = os.path.join(dest_dir, "temp.zip")
    urllib.request.urlretrieve(url, temp_path)
    
    with zipfile.ZipFile(temp_path, 'r') as zip_ref:
        if strip_components > 0:
            for member in zip_ref.namelist():
                parts = member.split('/')
                if len(parts) > strip_components:
                    new_path = '/'.join(parts[strip_components:])
                    if new_path:
                        target = os.path.join(dest_dir, new_path)
                        if member.endswith('/'):
                            os.makedirs(target, exist_ok=True)
                        else:
                            os.makedirs(os.path.dirname(target), exist_ok=True)
                            with zip_ref.open(member) as source:
                                with open(target, 'wb') as f:
                                    f.write(source.read())
        else:
            zip_ref.extractall(dest_dir)
    
    try:
        os.remove(temp_path)
    except PermissionError:
        pass


def clone_repo(repo_url, dest_dir, branch=None):
    import subprocess
    print(f"  Cloning {repo_url}...")
    cmd = ["git", "clone", "--depth", "1"]
    if branch:
        cmd.extend(["-b", branch])
    cmd.extend([repo_url, dest_dir])
    subprocess.run(cmd, check=True)


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
    if os.path.exists(os.path.join(dest_dir, ".git")):
        print("ktx already exists, skipping...")
        return
    
    if os.path.exists(dest_dir):
        print("Removing incomplete ktx directory...")
        shutil.rmtree(dest_dir, ignore_errors=True)
    
    print("Setting up ktx...")
    clone_repo("https://github.com/KhronosGroup/KTX-Software", dest_dir, branch="v4.4.2")
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
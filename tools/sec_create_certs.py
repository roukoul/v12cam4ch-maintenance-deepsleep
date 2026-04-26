#!/usr/bin/env python3
import os
import sys
import subprocess

# Configuration
CERT_DIR = os.path.join(os.path.dirname(__file__), '..', 'main', 'certs')
CERT_FILE = os.path.join(CERT_DIR, 'server_cert.pem')
KEY_FILE = os.path.join(CERT_DIR, 'server_key.pem')
CN = "aepbill.local"

def generate_certificates():
    if not os.path.exists(CERT_DIR):
        os.makedirs(CERT_DIR)
        print(f"Created directory: {CERT_DIR}")

    if os.path.exists(CERT_FILE) and os.path.exists(KEY_FILE):
        print("Certificates already exist. Skipping generation.")
        return

    print(f"Generating self-signed certificate for {CN}...")
    
    # OpenSSL command to generate key and cert
    openssl_bin = 'openssl'
    
    # Check for OpenSSL in specific Espressif path if not in PATH
    fallback_path = r'C:\Espressif\tools\idf-git\2.44.0\usr\bin\openssl.exe'
    if not os.path.exists(fallback_path):
        # Try another common location
        fallback_path = r'C:\Espressif\tools\idf-git\2.44.0\mingw64\bin\openssl.exe'
        
    try:
        subprocess.check_call([openssl_bin, 'version'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except (subprocess.CalledProcessError, FileNotFoundError):
        if os.path.exists(fallback_path):
            print(f"Using OpenSSL from: {fallback_path}")
            openssl_bin = fallback_path
        else:
            print(f"Warning: OpenSSL not found in PATH or {fallback_path}")

    cmd = [
        openssl_bin, 'req', '-x509', '-newkey', 'rsa:2048',
        '-keyout', KEY_FILE,
        '-out', CERT_FILE,
        '-days', '3650',
        '-nodes',
        '-subj', f'//CN={CN}//O=AepBill//C=FR' # Double slash for MinGW/Git Bash path conversion issues if needed
    ]
    
    # Fix for MinGW/Git Bash environment path conversion on Windows
    # But subprocess.check_call doesn't use the shell shell environment translation usually?
    # Let's use strict string for subject to avoid issues.
    # Actually, standard windows openssl takes /CN=... fine.
    # But let's stick to standard params. 
    # Create a temporary OpenSSL config file for SAN
    openssl_conf = f"""
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_req
prompt = no

[req_distinguished_name]
C = FR
O = AepBill
CN = {CN}

[v3_req]
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = {CN}
DNS.2 = localhost
IP.1 = 10.156.102.207
"""
    conf_file = os.path.join(CERT_DIR, 'openssl.cnf')
    with open(conf_file, 'w') as f:
        f.write(openssl_conf)

    cmd = [
        openssl_bin, 'req', '-x509', '-newkey', 'rsa:2048',
        '-keyout', KEY_FILE,
        '-out', CERT_FILE,
        '-days', '3650',
        '-nodes',
        '-config', conf_file,
        '-extensions', 'v3_req'
    ]

    try:
        subprocess.check_call(cmd)
        print("Certificates generated successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error generating certificates: {e}")
        # Try without -subj if that failed (interactive mode would be bad, but let's see)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: 'openssl' command not found. Please install OpenSSL.")
        sys.exit(1)

def file_to_c_array(filename, var_name):
    with open(filename, 'rb') as f:
        data = f.read()
    
    out = []
    out.append(f'const unsigned char {var_name}[] = {{')
    
    # Hex dump
    hex_data = [f'0x{b:02x}' for b in data]
    # Wrap at 12 bytes
    for i in range(0, len(hex_data), 12):
        line = ', '.join(hex_data[i:i+12])
        out.append(f'    {line},')
    
    # Null terminator just in case, though usually len is used
    out.append('    0x00');
    out.append('};')
    out.append(f'const unsigned int {var_name}_len = {len(data)};')
    return '\n'.join(out)

def generate_c_headers():
    print("Generating C header/source files...")
    
    c_source_file = os.path.join(CERT_DIR, 'certs_data.c')
    h_header_file = os.path.join(CERT_DIR, 'certs_data.h')

    # Generate .c content
    c_content = [
        '#include "certs_data.h"',
        '',
        file_to_c_array(CERT_FILE, 'server_cert_pem'),
        '',
        file_to_c_array(KEY_FILE, 'server_key_pem')
    ]
    
    with open(c_source_file, 'w') as f:
        f.write('\n'.join(c_content))
    
    # Generate .h content
    h_content = [
        '#ifndef CERTS_DATA_H',
        '#define CERTS_DATA_H',
        '',
        'extern const unsigned char server_cert_pem[];',
        'extern const unsigned int server_cert_pem_len;',
        '',
        'extern const unsigned char server_key_pem[];',
        'extern const unsigned int server_key_pem_len;',
        '',
        '#endif // CERTS_DATA_H'
    ]
    
    with open(h_header_file, 'w') as f:
        f.write('\n'.join(h_content))
        
    print(f"Generated {c_source_file} and {h_header_file}")

if __name__ == "__main__":
    generate_certificates()
    generate_c_headers()

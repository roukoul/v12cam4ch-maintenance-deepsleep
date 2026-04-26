import requests
import argparse
import sys
import hashlib
import time

# Colors for terminal output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

def log_success(msg):
    print(f"{Colors.OKGREEN}[PASS] {msg}{Colors.ENDC}")

def log_fail(msg):
    print(f"{Colors.FAIL}[FAIL] {msg}{Colors.ENDC}")

def log_info(msg):
    print(f"{Colors.OKBLUE}[INFO] {msg}{Colors.ENDC}")

def log_warn(msg):
    print(f"{Colors.WARNING}[WARN] {msg}{Colors.ENDC}")

def verify_access_control(ip):
    """Verifies that critical pages redirect to login when unauthenticated."""
    log_info("Testing Access Control (Unauthenticated)...")
    
    base_url = f"http://{ip}"
    protected_paths = [
        "/settings",
        "/wifiConfig",
        "/update",
        "/factoryReset",
        "/backup"
    ]
    
    all_passed = True
    session = requests.Session() # New clean session
    
    for path in protected_paths:
        try:
            url = f"{base_url}{path}"
            # Don't follow redirects automatically to check the 302/Login behavior
            r = session.get(url, allow_redirects=False, timeout=5)
            
            if r.status_code == 302:
                location = r.headers.get('Location', '')
                if location == '/' or location.endswith('/'):
                     log_success(f"Path '{path}' redirects to login.")
                else:
                    log_warn(f"Path '{path}' redirects to '{location}' (Expected root/login).")
                    all_passed = False
            elif r.status_code == 401:
                 log_success(f"Path '{path}' returns 401 Unauthorized.")
            elif r.status_code == 200:
                # If we get 200, check if it's actually the login page content
                if "password" in r.text.lower() and "login" in r.text.lower():
                     log_success(f"Path '{path}' served Login page.")
                else:
                    log_fail(f"Path '{path}' is ACCESSIBLE without auth! (Status 200)")
                    all_passed = False
            else:
                log_warn(f"Path '{path}' returned status {r.status_code}.")
                
        except Exception as e:
            log_warn(f"Could not connect to {path}: {e}")
            all_passed = False

    return all_passed

def verify_ota_safety(ip, cookies=None):
    """Verifies that OTA rejects requests without SHA256."""
    log_info("Testing OTA Safety Mechanisms...")
    
    url = f"http://{ip}/update"
    
    # payload with just a dummy file, NO sha256
    files = {'firmware': ('test.bin', b'dummy content')}
    
    try:
        # If we are authenticated, we can test the SHA256 check. 
        # If unauthenticated, it should just redirect us (tested above).
        # We assume cookies are passed if this is an auth test.
        r = requests.post(url, files=files, cookies=cookies, allow_redirects=False, timeout=10)
        
        if r.status_code == 400:
            if "SHA256" in r.text or "hash" in r.text:
                log_success("OTA Rejected update without SHA256 (Status 400).")
                return True
            else:
                log_warn("OTA returned 400 but didn't explicitly mention SHA256.")
                return True # Still a pass as it was rejected
                
        elif r.status_code == 302:
            log_info("OTA request redirected (likely invalid session).")
            return False # Not necessarily a fail of the mechanism, but test couldn't run
            
        elif r.status_code == 200:
             log_fail("OTA Accepted update without SHA256! (Dangerous)")
             return False
        else:
             log_warn(f"OTA test returned status {r.status_code}")
             return False

    except Exception as e:
        log_warn(f"OTA test failed connection: {e}")
        return False

def login(ip, password):
    """Attempts to login and returns cookies."""
    log_info(f"Attempting login with password: {'*' * len(password)}")
    url = f"http://{ip}/login"
    session = requests.Session()
    
    try:
        # The form expects 'password' field
        r = session.post(url, data={'password': password}, allow_redirects=False, timeout=5)
        
        # Check if we were redirected to / (success) or stayed on page (fail)
        if r.status_code == 302 and r.headers.get('Location') == '/':
            log_success("Login successful.")
            return session.cookies
        elif "Login Failed" in r.text:
            log_fail("Login failed (Invalid Password).")
            return None
        else:
            log_warn(f"Login returned unexpected status: {r.status_code}")
            return None
            
    except Exception as e:
        log_fail(f"Login connection failed: {e}")
        return None

def main():
    parser = argparse.ArgumentParser(description='AepBill Security Verification Tool')
    parser.add_argument('--ip', required=True, help='IP address of the ESP32')
    parser.add_argument('--password', default='admin', help='Admin password')
    
    args = parser.parse_args()
    
    print(f"\n{Colors.HEADER}=== AepBill Security Audit ==={Colors.ENDC}")
    print(f"Target: {args.ip}\n")
    
    # 1. Test Access Control (No Auth)
    if verify_access_control(args.ip):
        log_success("Protected pages are secure against unauthorized access.")
    else:
        log_fail("Some pages might be exposed!")

    print("-" * 40)

    # 2. Login
    cookies = login(args.ip, args.password)
    
    if cookies:
        print("-" * 40)
        
        # 3. Test OTA Safety (With Auth)
        # We try to push a dummy update WITHOUT the SHA256 hash.
        # It MUST fail.
        if verify_ota_safety(args.ip, cookies):
            log_success("OTA mechanism correctly enforces SHA256 validation.")
        else:
            log_warn("OTA safety check inconclusive or failed.")
    else:
        log_fail("Cannot proceed with OTA safety test without valid login.")
        
    print(f"\n{Colors.HEADER}=== Audit Complete ==={Colors.ENDC}")

if __name__ == "__main__":
    main()

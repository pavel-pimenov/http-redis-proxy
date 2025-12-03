#!/usr/bin/env python3
"""
Test script for HTTP Redis Proxy system
Includes functionality verification and load testing
"""

import subprocess
import time
import json
from typing import List, Tuple, Dict
import statistics
import sys
import os

# Check dependencies
def check_dependencies():
    """Check if required packages are installed."""
    required_packages = ['requests', 'aiohttp']
    missing_packages = []

    for package in required_packages:
        try:
            __import__(package.replace('-', '_'))
        except ImportError:
            missing_packages.append(package)

    if missing_packages:
        print(f"Missing dependencies: {', '.join(missing_packages)}")
        print("Please install them using: pip install -r requirements.txt")
        sys.exit(1)

# Install dependencies before other imports
check_dependencies()

import requests
import asyncio
import aiohttp

# Configuration
PROXY_URL = "http://localhost:8888"
NUM_REQUESTS = 50
CONCURRENT_REQUESTS = 10

class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    NC = '\033[0m'  # No Color

def run_command(cmd: List[str]) -> subprocess.CompletedProcess:
    """Run a shell command and return the result."""
    return subprocess.run(cmd, capture_output=True, text=True)

def start_services():
    """Start Docker Compose services."""
    print(f"{Colors.GREEN}Starting HTTP Redis Proxy services...{Colors.NC}")
    result = run_command(["docker-compose", "up", "-d"])
    if result.returncode != 0:
        print(f"{Colors.RED}Failed to start services: {result.stderr}{Colors.NC}")
        sys.exit(1)

def stop_services():
    """Stop Docker Compose services."""
    print(f"{Colors.GREEN}Stopping services...{Colors.NC}")
    run_command(["docker-compose", "down"])

def test_endpoint(url: str, expected_status: int = 200, description: str = "") -> bool:
    """Test a single endpoint with POST request."""
    print(f"Testing {description} ({url})... ", end="", flush=True)
    json_payload = {"test": "data", "number": 123}
    try:
        response = requests.post(url, json=json_payload, timeout=10)
        if response.status_code == expected_status:
            print(f"{Colors.GREEN}PASS{Colors.NC} (Status: {response.status_code})")
            return True
        else:
            print(f"{Colors.RED}FAIL{Colors.NC} (Status: {response.status_code}, Expected: {expected_status})")
            print(f"Response: {response.text}")
            return False
    except requests.RequestException as e:
        print(f"{Colors.RED}FAIL{Colors.NC} (Connection failed: {e})")
        return False

def test_json_endpoint(url: str, description: str = "") -> bool:
    """Test an endpoint that should return valid JSON with POST request."""
    print(f"Testing {description} ({url})... ", end="", flush=True)
    json_payload = {"test": "data", "number": 123}
    try:
        response = requests.post(url, json=json_payload, timeout=10)
        if response.status_code == 200:
            try:
                json.loads(response.text)
                print(f"{Colors.GREEN}PASS{Colors.NC} (Status: {response.status_code}, Valid JSON)")
                return True
            except json.JSONDecodeError:
                print(f"{Colors.RED}FAIL{Colors.NC} (Status: {response.status_code}, Invalid JSON)")
                print(f"Response: {response.text}")
                return False
        else:
            print(f"{Colors.RED}FAIL{Colors.NC} (Status: {response.status_code})")
            return False
    except requests.RequestException as e:
        print(f"{Colors.RED}FAIL{Colors.NC} (Connection failed: {e})")
        return False

async def make_async_request(session: aiohttp.ClientSession, request_id: int) -> Tuple[int, float, bool]:
    """Make a single async POST request with JSON payload and return timing info."""
    start_time = time.time()
    json_payload = {"test": "data", "number": 123}
    try:
        async with session.post(PROXY_URL, json=json_payload) as response:
            end_time = time.time()
            return request_id, end_time - start_time, response.status == 200
    except Exception:
        end_time = time.time()
        return request_id, end_time - start_time, False

async def run_load_test() -> Dict:
    """Run load testing with async requests."""
    print(f"Running load test with {NUM_REQUESTS} requests ({CONCURRENT_REQUESTS} concurrent)...")

    async with aiohttp.ClientSession() as session:
        tasks = []
        for i in range(NUM_REQUESTS):
            tasks.append(make_async_request(session, i + 1))

        results = await asyncio.gather(*tasks, return_exceptions=True)

    # Process results
    successful_requests = 0
    failed_requests = 0
    response_times = []

    for result in results:
        if isinstance(result, Exception):
            failed_requests += 1
        else:
            request_id, response_time, success = result
            if success:
                successful_requests += 1
                response_times.append(response_time * 1000)  # Convert to milliseconds
            else:
                failed_requests += 1

    stats = {
        'total_requests': len(results),
        'successful_requests': successful_requests,
        'failed_requests': failed_requests,
        'success_rate': (successful_requests / len(results)) * 100 if results else 0,
        'response_times': response_times
    }

    if response_times:
        stats.update({
            'avg_time': statistics.mean(response_times),
            'min_time': min(response_times),
            'max_time': max(response_times),
            'median_time': statistics.median(response_times)
        })

    return stats

def print_load_results(stats: Dict):
    """Print load test results."""
    print(f"\n{Colors.GREEN}=== Load Test Results ==={Colors.NC}")
    print(f"Total Requests: {stats['total_requests']}")
    print(f"Successful Requests: {stats['successful_requests']}")
    print(f"Failed Requests: {stats['failed_requests']}")
    print(f"Success Rate: {stats['success_rate']:.1f}%")

    if 'avg_time' in stats:
        print(f"Average Response Time: {stats['avg_time']:.2f}ms")
        print(f"Min Response Time: {stats['min_time']:.2f}ms")
        print(f"Max Response Time: {stats['max_time']:.2f}ms")
        print(f"Median Response Time: {stats['median_time']:.2f}ms")

def main():
    """Main test function."""
    try:
        # Start services
        start_services()

        # Wait for services to be ready
        print(f"{Colors.YELLOW}Waiting for services to be ready...{Colors.NC}")
        time.sleep(5)

        # Functionality tests
        print(f"\n{Colors.GREEN}=== Functionality Tests ==={Colors.NC}")

        all_passed = True
        all_passed &= test_endpoint(f"{PROXY_URL}/health", 200, "Proxy Health Check")
        all_passed &= test_json_endpoint(f"{PROXY_URL}/", "Proxy Main Endpoint")

        # Load testing
        print(f"\n{Colors.GREEN}=== Load Testing ==={Colors.NC}")
        stats = asyncio.run(run_load_test())
        print_load_results(stats)

        # Determine overall success
        load_passed = stats['success_rate'] >= 95.0 and stats['failed_requests'] == 0

        if all_passed and load_passed:
            print(f"\n{Colors.GREEN}✅ All tests passed! System is working correctly.{Colors.NC}")
            return 0
        else:
            print(f"\n{Colors.RED}❌ Some tests failed. Check the output above.{Colors.NC}")
            return 1

    finally:
        stop_services()

if __name__ == "__main__":
    sys.exit(main())

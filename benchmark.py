#!/usr/bin/env python3
"""
Benchmark script for load testing l2-proxy on port 8888
"""

import asyncio
import aiohttp
import time
import statistics
import sys
import argparse

# Configuration defaults
DEFAULT_URL = "http://localhost:8888/"
DEFAULT_NUM_REQUESTS = 1000
DEFAULT_CONCURRENT = 50

class Colors:
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[1;33m'
    NC = '\033[0m'  # No Color

async def make_request(session: aiohttp.ClientSession, request_id: int, url: str) -> tuple:
    """Make a single HTTP request and return timing info."""
    start_time = time.time()
    try:
        async with session.post(url) as response:
            end_time = time.time()
            response_time = (end_time - start_time) * 1000  # Convert to milliseconds
            success = response.status == 200
            return request_id, response_time, success, response.status
    except Exception as e:
        end_time = time.time()
        response_time = (end_time - start_time) * 1000
        return request_id, response_time, False, 0

async def run_benchmark(url: str, num_requests: int, concurrent: int) -> dict:
    """Run the benchmark with specified parameters."""
    print(f"{Colors.GREEN}Starting benchmark...{Colors.NC}")
    print(f"URL: {url}")
    print(f"Total requests: {num_requests}")
    print(f"Concurrent requests: {concurrent}")
    print("-" * 50)

    semaphore = asyncio.Semaphore(concurrent)

    async def limited_request(session, request_id):
        async with semaphore:
            return await make_request(session, request_id, url)

    async with aiohttp.ClientSession() as session:
        tasks = [limited_request(session, i + 1) for i in range(num_requests)]
        results = await asyncio.gather(*tasks, return_exceptions=True)

    # Process results
    successful_requests = 0
    failed_requests = 0
    response_times = []
    status_codes = {}

    for result in results:
        if isinstance(result, Exception):
            failed_requests += 1
        else:
            request_id, response_time, success, status = result
            if success:
                successful_requests += 1
            else:
                failed_requests += 1

            response_times.append(response_time)

            if status not in status_codes:
                status_codes[status] = 0
            status_codes[status] += 1

    # Calculate statistics
    stats = {
        'total_requests': len(results),
        'successful_requests': successful_requests,
        'failed_requests': failed_requests,
        'success_rate': (successful_requests / len(results)) * 100 if results else 0,
        'response_times': response_times,
        'status_codes': status_codes
    }

    if response_times:
        stats.update({
            'avg_time': statistics.mean(response_times),
            'min_time': min(response_times),
            'max_time': max(response_times),
            'median_time': statistics.median(response_times),
            'p95_time': statistics.quantiles(response_times, n=20)[18] if len(response_times) >= 20 else max(response_times),
            'p99_time': statistics.quantiles(response_times, n=100)[98] if len(response_times) >= 100 else max(response_times)
        })

    return stats

def print_results(stats: dict):
    """Print benchmark results in a formatted way."""
    print(f"\n{Colors.GREEN}=== Benchmark Results ==={Colors.NC}")
    print(f"Total Requests: {stats['total_requests']}")
    print(f"Successful Requests: {stats['successful_requests']}")
    print(f"Failed Requests: {stats['failed_requests']}")
    print(f"Success Rate: {stats['success_rate']:.2f}%")

    if 'avg_time' in stats:
        print(f"\n{Colors.YELLOW}Response Time Statistics (ms):{Colors.NC}")
        print(f"Average: {stats['avg_time']:.2f}")
        print(f"Min: {stats['min_time']:.2f}")
        print(f"Max: {stats['max_time']:.2f}")
        print(f"Median: {stats['median_time']:.2f}")
        print(f"95th percentile: {stats['p95_time']:.2f}")
        print(f"99th percentile: {stats['p99_time']:.2f}")

    if stats['status_codes']:
        print(f"\n{Colors.YELLOW}HTTP Status Codes:{Colors.NC}")
        for status, count in sorted(stats['status_codes'].items()):
            print(f"  {status}: {count}")

    print("-" * 50)

def main():
    """Main function with argument parsing."""
    parser = argparse.ArgumentParser(description='Benchmark l2-proxy on port 8888')
    parser.add_argument('-u', '--url', default=DEFAULT_URL,
                       help=f'Target URL (default: {DEFAULT_URL})')
    parser.add_argument('-n', '--requests', type=int, default=DEFAULT_NUM_REQUESTS,
                       help=f'Number of requests (default: {DEFAULT_NUM_REQUESTS})')
    parser.add_argument('-c', '--concurrent', type=int, default=DEFAULT_CONCURRENT,
                       help=f'Number of concurrent requests (default: {DEFAULT_CONCURRENT})')

    args = parser.parse_args()

    try:
        stats = asyncio.run(run_benchmark(args.url, args.requests, args.concurrent))
        print_results(stats)

        # Exit with error code if success rate is too low
        if stats['success_rate'] < 95.0:
            print(f"{Colors.RED}Warning: Low success rate ({stats['success_rate']:.2f}%){Colors.NC}")
            sys.exit(1)

    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Benchmark interrupted by user{Colors.NC}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.NC}")
        sys.exit(1)

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
meowline - A simple status line for your terminal
"""

import sys
import os
import time
from datetime import datetime


def get_username():
    """Get current username."""
    return os.environ.get('USER', os.environ.get('USERNAME', 'user'))


def get_hostname():
    """Get hostname."""
    import socket
    return socket.gethostname()


def get_current_path():
    """Get current working directory."""
    cwd = os.getcwd()
    home = os.path.expanduser('~')
    if cwd.startswith(home):
        return '~' + cwd[len(home):]
    return cwd


def get_time():
    """Get current time."""
    return datetime.now().strftime('%H:%M:%S')


def get_date():
    """Get current date."""
    return datetime.now().strftime('%Y-%m-%d')


def create_status_line(components=None):
    """
    Create a status line with specified components.
    
    Args:
        components: List of component names to include.
                   Available: 'user', 'host', 'path', 'time', 'date'
    
    Returns:
        Formatted status line string
    """
    if components is None:
        components = ['user', 'host', 'path', 'time']
    
    parts = []
    
    for component in components:
        if component == 'user':
            parts.append(f"👤 {get_username()}")
        elif component == 'host':
            parts.append(f"🖥️  {get_hostname()}")
        elif component == 'path':
            parts.append(f"📁 {get_current_path()}")
        elif component == 'time':
            parts.append(f"🕐 {get_time()}")
        elif component == 'date':
            parts.append(f"📅 {get_date()}")
    
    return ' | '.join(parts)


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='meowline - A simple status line for your terminal',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  meowline                    # Show default status line
  meowline --components user host path time
  meowline --watch            # Update every second
        """
    )
    
    parser.add_argument(
        '--components',
        nargs='+',
        choices=['user', 'host', 'path', 'time', 'date'],
        help='Components to include in status line'
    )
    
    parser.add_argument(
        '--watch',
        action='store_true',
        help='Continuously update the status line'
    )
    
    parser.add_argument(
        '--interval',
        type=float,
        default=1.0,
        help='Update interval in seconds (default: 1.0)'
    )
    
    args = parser.parse_args()
    
    try:
        if args.watch:
            # Continuously update
            while True:
                status = create_status_line(args.components)
                # Clear line and print status
                sys.stdout.write('\r' + status)
                sys.stdout.flush()
                time.sleep(args.interval)
        else:
            # Print once
            status = create_status_line(args.components)
            print(status)
    except KeyboardInterrupt:
        if args.watch:
            print()  # New line after ^C
        sys.exit(0)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
BabaChain Build Failure Notification System
Sends structured notifications with actionable information when builds fail
"""

import os
import sys
import json
import requests
from datetime import datetime
from typing import Dict, List, Optional
from dataclasses import dataclass
from enum import Enum

class NotificationChannel(Enum):
    GITHUB_ISSUE = "github_issue"
    SLACK = "slack"
    DISCORD = "discord"
    EMAIL = "email"
    WEBHOOK = "webhook"

@dataclass
class NotificationConfig:
    channel: NotificationChannel
    enabled: bool
    config: Dict

@dataclass
class BuildFailureNotification:
    platform: str
    build_phase: str
    error_count: int
    warning_count: int
    summary: str
    errors: List[Dict]
    commit_sha: str
    branch: str
    build_url: str
    timestamp: str

class FailureNotifier:
    def __init__(self, config_file: Optional[str] = None):
        self.config = self._load_config(config_file)
        
    def _load_config(self, config_file: Optional[str]) -> Dict[str, NotificationConfig]:
        """Load notification configuration"""
        default_config = {
            "github_issue": NotificationConfig(
                channel=NotificationChannel.GITHUB_ISSUE,
                enabled=os.getenv('GITHUB_ACTIONS') == 'true',
                config={
                    "token": os.getenv('GITHUB_TOKEN'),
                    "repository": os.getenv('GITHUB_REPOSITORY'),
                    "labels": ["build-failure", "ci/cd"]
                }
            ),
            "slack": NotificationConfig(
                channel=NotificationChannel.SLACK,
                enabled=bool(os.getenv('SLACK_WEBHOOK_URL')),
                config={
                    "webhook_url": os.getenv('SLACK_WEBHOOK_URL'),
                    "channel": os.getenv('SLACK_CHANNEL', '#builds'),
                    "username": "BabaChain CI"
                }
            ),
            "discord": NotificationConfig(
                channel=NotificationChannel.DISCORD,
                enabled=bool(os.getenv('DISCORD_WEBHOOK_URL')),
                config={
                    "webhook_url": os.getenv('DISCORD_WEBHOOK_URL'),
                    "username": "BabaChain CI"
                }
            )
        }
        
        if config_file and os.path.exists(config_file):
            with open(config_file, 'r') as f:
                custom_config = json.load(f)
                # Merge with default config
                for key, value in custom_config.items():
                    if key in default_config:
                        default_config[key].config.update(value.get('config', {}))
                        default_config[key].enabled = value.get('enabled', default_config[key].enabled)
        
        return default_config
    
    def notify_failure(self, notification: BuildFailureNotification):
        """Send failure notifications through all enabled channels"""
        for channel_name, config in self.config.items():
            if not config.enabled:
                continue
                
            try:
                if config.channel == NotificationChannel.GITHUB_ISSUE:
                    self._notify_github_issue(notification, config.config)
                elif config.channel == NotificationChannel.SLACK:
                    self._notify_slack(notification, config.config)
                elif config.channel == NotificationChannel.DISCORD:
                    self._notify_discord(notification, config.config)
                    
                print(f"✅ Notification sent via {channel_name}")
                
            except Exception as e:
                print(f"❌ Failed to send notification via {channel_name}: {e}")
    
    def _notify_github_issue(self, notification: BuildFailureNotification, config: Dict):
        """Create GitHub issue for build failure"""
        if not config.get('token') or not config.get('repository'):
            raise ValueError("GitHub token and repository required")
        
        # Check if similar issue already exists
        existing_issue = self._find_existing_github_issue(notification, config)
        if existing_issue:
            self._update_github_issue(existing_issue, notification, config)
            return
        
        # Create new issue
        title = f"🚨 Build Failure: {notification.platform} ({notification.build_phase})"
        
        body = self._generate_github_issue_body(notification)
        
        headers = {
            'Authorization': f'token {config["token"]}',
            'Accept': 'application/vnd.github.v3+json'
        }
        
        data = {
            'title': title,
            'body': body,
            'labels': config.get('labels', [])
        }
        
        response = requests.post(
            f'https://api.github.com/repos/{config["repository"]}/issues',
            headers=headers,
            json=data
        )
        response.raise_for_status()
    
    def _find_existing_github_issue(self, notification: BuildFailureNotification, config: Dict) -> Optional[Dict]:
        """Find existing GitHub issue for similar build failure"""
        headers = {
            'Authorization': f'token {config["token"]}',
            'Accept': 'application/vnd.github.v3+json'
        }
        
        # Search for open issues with build-failure label
        params = {
            'labels': 'build-failure',
            'state': 'open',
            'sort': 'updated',
            'direction': 'desc'
        }
        
        response = requests.get(
            f'https://api.github.com/repos/{config["repository"]}/issues',
            headers=headers,
            params=params
        )
        response.raise_for_status()
        
        issues = response.json()
        
        # Look for issue with same platform in title
        for issue in issues:
            if notification.platform in issue['title']:
                return issue
        
        return None
    
    def _update_github_issue(self, issue: Dict, notification: BuildFailureNotification, config: Dict):
        """Update existing GitHub issue with new failure information"""
        headers = {
            'Authorization': f'token {config["token"]}',
            'Accept': 'application/vnd.github.v3+json'
        }
        
        comment_body = f"""
## 🔄 Build Failure Update

**Timestamp:** {notification.timestamp}
**Commit:** {notification.commit_sha}
**Branch:** {notification.branch}
**Build URL:** {notification.build_url}

**Error Summary:**
{notification.summary}

**Error Count:** {notification.error_count}
**Warning Count:** {notification.warning_count}

---
*This is an automated update from the BabaChain CI system.*
"""
        
        data = {'body': comment_body}
        
        response = requests.post(
            f'https://api.github.com/repos/{config["repository"]}/issues/{issue["number"]}/comments',
            headers=headers,
            json=data
        )
        response.raise_for_status()
    
    def _generate_github_issue_body(self, notification: BuildFailureNotification) -> str:
        """Generate GitHub issue body for build failure"""
        body = f"""
## 🚨 Build Failure Report

**Platform:** {notification.platform}
**Build Phase:** {notification.build_phase}
**Timestamp:** {notification.timestamp}
**Commit:** {notification.commit_sha}
**Branch:** {notification.branch}
**Build URL:** {notification.build_url}

### 📊 Summary
{notification.summary}

**Errors:** {notification.error_count}
**Warnings:** {notification.warning_count}

### 🔍 Top Errors
"""
        
        # Add top 5 errors
        for i, error in enumerate(notification.errors[:5], 1):
            body += f"""
#### Error {i}: {error.get('context', 'Unknown Error')}
- **Category:** {error.get('category', 'unknown')}
- **Severity:** {error.get('severity', 'unknown')}
- **Message:** `{error.get('message', '')}`
- **Suggestion:** {error.get('suggestion', 'No suggestion available')}
"""
            if error.get('documentation_link'):
                body += f"- **Documentation:** {error['documentation_link']}\n"
        
        body += f"""

### 🛠️ Troubleshooting Steps

1. **Check the build logs** in the [GitHub Actions run]({notification.build_url})
2. **Review the error messages** above for specific issues
3. **Check dependencies** - ensure all required packages are installed
4. **Verify platform compatibility** - some features may not be available on all platforms
5. **Check recent changes** - review commits since the last successful build

### 📚 Documentation Links

- [Build Instructions](https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-unix.md)
- [Cross-Platform Building](https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-cross-platform.md)
- [Troubleshooting Guide](https://github.com/Baba-Chain/BabaChain/blob/main/doc/build-troubleshooting.md)

### 🤖 Automation Info

This issue was automatically created by the BabaChain CI system.
- **Build ID:** {notification.commit_sha[:8]}
- **Generated:** {notification.timestamp}

---
*Please assign this issue to the appropriate team member and add relevant labels.*
"""
        
        return body
    
    def _notify_slack(self, notification: BuildFailureNotification, config: Dict):
        """Send Slack notification for build failure"""
        webhook_url = config.get('webhook_url')
        if not webhook_url:
            raise ValueError("Slack webhook URL required")
        
        # Determine color based on error count
        color = "danger" if notification.error_count > 0 else "warning"
        
        # Create Slack message
        message = {
            "channel": config.get('channel', '#builds'),
            "username": config.get('username', 'BabaChain CI'),
            "icon_emoji": ":warning:",
            "attachments": [
                {
                    "color": color,
                    "title": f"🚨 Build Failure: {notification.platform}",
                    "title_link": notification.build_url,
                    "fields": [
                        {
                            "title": "Platform",
                            "value": notification.platform,
                            "short": True
                        },
                        {
                            "title": "Build Phase",
                            "value": notification.build_phase,
                            "short": True
                        },
                        {
                            "title": "Errors",
                            "value": str(notification.error_count),
                            "short": True
                        },
                        {
                            "title": "Warnings",
                            "value": str(notification.warning_count),
                            "short": True
                        },
                        {
                            "title": "Commit",
                            "value": f"`{notification.commit_sha[:8]}`",
                            "short": True
                        },
                        {
                            "title": "Branch",
                            "value": notification.branch,
                            "short": True
                        }
                    ],
                    "text": notification.summary,
                    "footer": "BabaChain CI",
                    "ts": int(datetime.fromisoformat(notification.timestamp.replace('Z', '+00:00')).timestamp())
                }
            ]
        }
        
        response = requests.post(webhook_url, json=message)
        response.raise_for_status()
    
    def _notify_discord(self, notification: BuildFailureNotification, config: Dict):
        """Send Discord notification for build failure"""
        webhook_url = config.get('webhook_url')
        if not webhook_url:
            raise ValueError("Discord webhook URL required")
        
        # Determine color based on error count
        color = 0xFF0000 if notification.error_count > 0 else 0xFFA500  # Red or Orange
        
        # Create Discord embed
        embed = {
            "title": f"🚨 Build Failure: {notification.platform}",
            "description": notification.summary,
            "color": color,
            "url": notification.build_url,
            "fields": [
                {
                    "name": "Platform",
                    "value": notification.platform,
                    "inline": True
                },
                {
                    "name": "Build Phase",
                    "value": notification.build_phase,
                    "inline": True
                },
                {
                    "name": "Errors",
                    "value": str(notification.error_count),
                    "inline": True
                },
                {
                    "name": "Warnings",
                    "value": str(notification.warning_count),
                    "inline": True
                },
                {
                    "name": "Commit",
                    "value": f"`{notification.commit_sha[:8]}`",
                    "inline": True
                },
                {
                    "name": "Branch",
                    "value": notification.branch,
                    "inline": True
                }
            ],
            "footer": {
                "text": "BabaChain CI"
            },
            "timestamp": notification.timestamp
        }
        
        # Add top errors as fields
        for i, error in enumerate(notification.errors[:3], 1):
            embed["fields"].append({
                "name": f"Error {i}",
                "value": f"**{error.get('context', 'Unknown')}**\n{error.get('suggestion', 'No suggestion')}",
                "inline": False
            })
        
        message = {
            "username": config.get('username', 'BabaChain CI'),
            "embeds": [embed]
        }
        
        response = requests.post(webhook_url, json=message)
        response.raise_for_status()

def main():
    if len(sys.argv) < 2:
        print("Usage: failure-notifier.py <build_report_json> [config_file]")
        sys.exit(1)
    
    report_file = sys.argv[1]
    config_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    if not os.path.exists(report_file):
        print(f"Error: Build report file {report_file} not found")
        sys.exit(1)
    
    # Load build report
    with open(report_file, 'r') as f:
        report_data = json.load(f)
    
    # Create notification
    notification = BuildFailureNotification(
        platform=report_data.get('platform', 'unknown'),
        build_phase=report_data.get('build_phase', 'unknown'),
        error_count=len(report_data.get('errors', [])),
        warning_count=len(report_data.get('warnings', [])),
        summary=report_data.get('summary', 'Build failed'),
        errors=report_data.get('errors', []),
        commit_sha=os.getenv('GITHUB_SHA', 'unknown'),
        branch=os.getenv('GITHUB_REF_NAME', 'unknown'),
        build_url=f"https://github.com/{os.getenv('GITHUB_REPOSITORY', 'unknown')}/actions/runs/{os.getenv('GITHUB_RUN_ID', 'unknown')}",
        timestamp=report_data.get('timestamp', datetime.now().isoformat())
    )
    
    # Send notifications
    notifier = FailureNotifier(config_file)
    notifier.notify_failure(notification)
    
    print("✅ Failure notifications sent successfully")

if __name__ == "__main__":
    main()
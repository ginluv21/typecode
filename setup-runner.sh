#!/bin/bash
set -e

RUNNER_VERSION="2.334.0"
RUNNER_USER="github-runner"
REPO="https://github.com/ginluv21/typecode"

if [ -z "$1" ]; then
    echo "Usage: $0 <REGISTRATION_TOKEN>"
    echo ""
    echo "Get token at: https://github.com/ginluv21/typecode/settings/actions/runners/new"
    exit 1
fi

TOKEN="$1"

apt-get update -qq
apt-get install -y gcc libncurses-dev python3-pip curl tar
pip3 install -q gcovr

if ! id "$RUNNER_USER" &>/dev/null; then
    useradd -m -s /bin/bash "$RUNNER_USER"
fi

RUNNER_DIR="/home/$RUNNER_USER/actions-runner"
mkdir -p "$RUNNER_DIR"

curl -sL "https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz" \
    | tar -xz -C "$RUNNER_DIR"

chown -R "$RUNNER_USER:$RUNNER_USER" "$RUNNER_DIR"

su - "$RUNNER_USER" -c "
    cd $RUNNER_DIR
    ./config.sh --url $REPO --token $TOKEN --name typecode-runner --unattended --replace
"

cat > /etc/systemd/system/github-runner.service <<EOF
[Unit]
Description=GitHub Actions Runner
After=network.target

[Service]
User=$RUNNER_USER
WorkingDirectory=$RUNNER_DIR
ExecStart=$RUNNER_DIR/run.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable github-runner
systemctl start github-runner

echo ""
echo "Runner started. Status:"
systemctl status github-runner --no-pager

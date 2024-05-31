#!/usr/bin/python3
import argparse
import sys
import paramiko

class SshTool:
    def __init__(self, host, user='root', password='cvitek'):
        self.host = host
        self.user = user
        self.password = password
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.__connect()

    def __connect(self, try_cnt=5):
        for i in range(try_cnt):
            try:
                self.ssh.connect(self.host, '22', self.user, self.password, timeout=20)
                return
            except Exception as e:
                print(e)
                continue
        raise Exception("connection failed")

    def exec(self, command):
        _, stdout, stderr = self.ssh.exec_command(command)
        for line in stdout.readlines():
            print(line.strip())
        for line in stderr.readlines():
            print(line.strip())
        err = stdout.channel.recv_exit_status()
        print("ret:{}".format(err))
        return err

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="SSH client")
    parser.add_argument("-b", "--target", type=str, required=True,
                        help="specify ip of the target board")
    parser.add_argument("-u", "--user", type=str, default="root",
                        help="specify the user name to login")
    parser.add_argument("-p", "--password", type=str, default="cvitek",
                        help="password to login")
    parser.add_argument("-t", "--timeout", type=int, default=0,
                        help="timeout time to connect")
    parser.add_argument("-e", "--exec", type=str,
                        help="shell command to be executed on target board")

    args = parser.parse_args()
    ssh = SshTool(host=args.target, user=args.user, password=args.password)
    ret = ssh.exec(args.exec)
    sys.exit(ret)

import websocket
import json
import pprint
import base64
import cv2
import numpy as np

try:
    import thread
except ImportError:
    import _thread as thread
import time
import sys


test_list = [
    "testing_json/awbinfo_write_module.json",
    "testing_json/gamma_write_module.json",
    "testing_json/dehaze_write_module.json"
]


def on_message(ws, message):
    with open("test.json", "w") as f:
        f.write(message)
    data = json.loads(message)
    print(json.dumps(data, indent=4))
    print(data["cmd"])
    if data["cmd"] == "IMAGE_DATA":
        print(data["timestamp"])
        frame = data["data"]
        frame = base64.b64decode(frame)
        arr = np.fromstring(frame, np.uint8)
        # img_np = cv2.imdecode(arr, cv2.IMREAD_COLOR)

        # cv2.imshow("read_image", img_np)
        # cv2.waitKey(1)
    if data["cmd"] == "GET_SINGLE_RAW_DATA":
        with open("test.yuv", "wb") as f:
            frame = data["data"]
            frame = base64.b64decode(frame)
            f.write(frame)

def on_error(ws, error):
    print("ERR", error)


def on_close(ws):
    print("Close")


def sending_json():
    for f in test_list:
        with open(f, "r") as reader:
            data = json.load(reader)
            ws.send(json.dumps(data))
            print(json.dumps(data))
        time.sleep(3)
    ws.close()


def on_open(ws):
    thread.start_new_thread(sending_json, ())


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("{} [server ip address] ".format(sys.argv[0]))
        sys.exit(1)
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp(
        "ws://{}:5566".format(sys.argv[1]),
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
    )
    ws.on_open = on_open
    ws.run_forever()

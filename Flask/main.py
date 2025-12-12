from flask import Flask, render_template, jsonify, request, Response
from flask_sock import Sock
from enum import Enum
from models.CrowdAnalysis import RunYolo
import requests
import logging
import base64
import io
from PIL import Image

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

peopleCount = ""
imagePath = ""
system_images = {}

app = Flask(__name__)
sock = Sock(app)
class CrowdLevel(Enum):
    LOW = "low"
    MEDIUM = "medium"
    DENSE = "dense"
    FULL = "full" 

Systems = [
    {"id" : 1, "systemName" : "System 1", "crowdLevel": "", "peopleCount": "", "maxPeople": ""}
]

@app.route("/")
def mainPage():
    return render_template("mainPage.html", systems=Systems)

@app.route("/api/systems/<int:system_id>", methods=["PATCH"])
def update_system(system_id):
    system = next((s for s in Systems if s["id"] == system_id), None)
    if not system:
        return jsonify({"error": "System not found"}), 404
    data = request.get_json()
    if data.get("maxPeople") is not None:
        system["maxPeople"] = data.get("maxPeople")
    if data.get("image") is not None:
        img_data = data.get("image")
        logger.info(f"[FLASK] Received image for system {system_id}: {len(img_data)} chars")
        if img_data.startswith("data:image"):
            img_data = img_data.split(",")[1]
        system_images[system_id] = img_data
        logger.info(f"[FLASK] Image stored successfully for system {system_id}")
        try:
            img_bytes = base64.b64decode(img_data)
            img = Image.open(io.BytesIO(img_bytes))
            temp_path = f"temp_{system_id}.jpg"
            img.save(temp_path)
            crowdLevel, standing, sitting, peopleCount = RunYolo(temp_path)
            system["peopleCount"] = peopleCount
            maxPeople = int(system["maxPeople"]) if system["maxPeople"] else 0
            if maxPeople > 0 and peopleCount >= maxPeople:
                system["crowdLevel"] = CrowdLevel.FULL.value
            else:
                system["crowdLevel"] = crowdLevel
            logger.info(f"[FLASK] Image processed: {peopleCount} people detected")
        except Exception as e:
            logger.error(f"[FLASK] Failed to process image: {e}")
    return jsonify(system), 200

@app.route("/api/systems", methods=["GET"])
def get_systems():
    return jsonify(Systems), 200

@app.route("/api/image/<int:system_id>", methods=["GET"])
def get_image(system_id):
    if system_id in system_images:
        return jsonify({"image": system_images[system_id]}), 200
    return jsonify({"error": "No image"}), 404

@app.route("/api/test-esp32-connection", methods=["GET"])
def test_esp32_connection():
    logger.info("=== Testing ESP32 Connection ===")
    result = {
        "esp32_url": "ws://192.168.4.1/ws",
        "connection_status": "unknown",
        "error": None,
        "details": []
    }
    
    try:
        import websocket
        import socket
        
        logger.info("Step 1: Testing network connectivity to 192.168.4.1...")
        result["details"].append("Testing network connectivity...")
        
        try:
            sock_test = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock_test.settimeout(3)
            sock_test.connect(("192.168.4.1", 80))
            sock_test.close()
            result["details"].append("✓ Network reachable at 192.168.4.1:80")
            logger.info("Network is reachable")
        except socket.timeout:
            result["details"].append("✗ Network timeout - ESP32 not responding")
            result["connection_status"] = "timeout"
            logger.error("Network timeout")
            return jsonify(result), 200
        except socket.error as e:
            result["details"].append(f"✗ Network error: {e}")
            result["connection_status"] = "network_error"
            logger.error(f"Network error: {e}")
            return jsonify(result), 200
        
        logger.info("Step 2: Testing WebSocket connection...")
        result["details"].append("Testing WebSocket connection...")
        
        ws = websocket.WebSocket()
        ws.settimeout(5)
        ws.connect("ws://192.168.4.1/ws")
        
        result["details"].append("✓ WebSocket connected successfully")
        result["connection_status"] = "connected"
        logger.info("WebSocket connected successfully")
        
        logger.info("Step 3: Waiting for data...")
        result["details"].append("Waiting for data from ESP32...")
        
        try:
            data = ws.recv()
            data_size = len(data) if data else 0
            result["details"].append(f"✓ Received {data_size} bytes of data")
            result["connection_status"] = "receiving_data"
            result["first_frame_size"] = data_size
            logger.info(f"Received data: {data_size} bytes")
        except websocket.WebSocketTimeoutException:
            result["details"].append("⚠ Connected but no data received (timeout)")
            result["connection_status"] = "connected_no_data"
            logger.warning("Connected but no data received")
        
        ws.close()
        
    except websocket.WebSocketException as ws_error:
        result["connection_status"] = "websocket_error"
        result["error"] = str(ws_error)
        result["details"].append(f"✗ WebSocket error: {ws_error}")
        logger.error(f"WebSocket error: {ws_error}")
    except Exception as e:
        result["connection_status"] = "error"
        result["error"] = str(e)
        result["details"].append(f"✗ Error: {e}")
        logger.error(f"Error: {e}")
    
    logger.info(f"Test result: {result['connection_status']}")
    return jsonify(result), 200

@sock.route('/stream')
def stream(ws):
    esp32_ws = None
    logger.info("=== WebSocket /stream endpoint called ===")
    logger.info(f"Client connected from: {request.remote_addr}")
    
    try:
        import websocket
        esp32_url = "ws://192.168.4.1/ws"
        logger.info(f"Attempting to connect to ESP32 at: {esp32_url}")
        
        esp32_ws = websocket.WebSocket()
        esp32_ws.settimeout(5)
        esp32_ws.connect(esp32_url)
        
        logger.info("Successfully connected to ESP32 WebSocket!")
        
        frame_count = 0
        while True:
            try:
                data = esp32_ws.recv()
                frame_count += 1
                data_size = len(data) if data else 0
                
                logger.debug(f"Frame {frame_count}: Received {data_size} bytes from ESP32")
                
                if data:
                    ws.send(data)
                    logger.debug(f"Frame {frame_count}: Forwarded to client successfully")
                else:
                    logger.warning(f"Frame {frame_count}: Received empty data from ESP32")
                    
            except websocket.WebSocketTimeoutException:
                logger.warning("Timeout waiting for frame from ESP32")
            except Exception as recv_error:
                logger.error(f"Error receiving/sending frame: {recv_error}")
                break
                
    except websocket.WebSocketException as ws_error:
        logger.error(f"WebSocket connection error: {ws_error}")
        logger.error("Make sure ESP32 is powered on and accessible at 192.168.4.1")
    except Exception as e:
        logger.error(f"Unexpected error in stream: {e}")
        import traceback
        logger.error(traceback.format_exc())
    finally:
        if esp32_ws:
            try:
                esp32_ws.close()
                logger.info("ESP32 WebSocket connection closed")
            except:
                pass
        logger.info(f"=== WebSocket /stream session ended. Total frames: {frame_count if 'frame_count' in locals() else 0} ===")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
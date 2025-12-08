from flask import Flask, render_template, jsonify, request
from enum import Enum
from models.CrowdAnalysis import RunYolo

peopleCount = ""
imagePath = ""

app = Flask(__name__)
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
    imagePath = data.get("image")
    system["peopleCount"] = data.get("peopleCount")
    system["crowdLevel"] = RunYolo("test2.jpeg")[0]
    return jsonify(system), 200

@app.route("/api/systems", methods=["GET"])
def get_systems():
    """Get all systems data"""
    return jsonify(Systems), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
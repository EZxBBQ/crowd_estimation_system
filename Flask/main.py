from flask import Flask, render_template, jsonify, request
from enum import Enum

app = Flask(__name__)
class CrowdLevel(Enum):
    LOW = "low"
    MEDIUM = "medium"
    DENSE = "dense"
    FULL = "full" 

Systems = [
    {"id" : 1, "systemName": "System 1", "crowdLevel": CrowdLevel.LOW.name, "peopleCount": 10, "maxPeople": 50},
    {"id" : 2, "systemName": "System 2", "crowdLevel": CrowdLevel.MEDIUM.name, "peopleCount": 23, "maxPeople": 40},
    {"id" : 3, "systemName": "System 3", "crowdLevel": CrowdLevel.DENSE.name, "peopleCount": 35, "maxPeople": 45},
    {"id" : 4, "systemName": "System 4", "crowdLevel": CrowdLevel.FULL.name, "peopleCount": 5, "maxPeople": 30}
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
    print("payload:", data)
    
    # Update allowed fields only
    if "crowdLevel" in data:
        # Validate crowd level
        valid_levels = [level.name for level in CrowdLevel]
        if data["crowdLevel"].upper() in valid_levels:
            system["crowdLevel"] = data["crowdLevel"].upper()
        else:
            return jsonify({"error": f"Invalid crowdLevel. Must be one of: {valid_levels}"}), 400
    
    if "peopleCount" in data:
        try:
            system["peopleCount"] = int(data["peopleCount"])
        except ValueError:
            return jsonify({"error": "peopleCount must be an integer"}), 400
    
    if "maxPeople" in data:
        try:
            system["maxPeople"] = int(data["maxPeople"])
        except ValueError:
            return jsonify({"error": "maxPeople must be an integer"}), 400
    
    return jsonify(system), 200

@app.route("/api/systems", methods=["GET"])
def get_systems():
    """Get all systems data"""
    return jsonify(Systems), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
from flask import Flask, render_template
from enum import Enum

app = Flask(__name__)
class CrowdLevel(Enum):
    LOW = "low"
    MEDIUM = "medium"
    DENSE = "dense"
    FULL = "full"

@app.route("/")
def mainPage():
    crowd_level = CrowdLevel.LOW.value
    people_count = 0
    max_people = 50  
    return render_template("mainPage.html", crowdLevel =crowd_level, peopleCount=people_count, maxPeople=max_people)

if __name__ == "__main__":
    app.run(debug=True)
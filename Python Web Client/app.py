# References
# https://www.freecodecamp.org/news/how-to-get-started-with-firebase-using-python/
# https://vegibit.com/how-to-use-forms-in-python-flask/

import firebase_admin
import os
from flask import Flask, render_template, request
from firebase_admin import db

# Place the Firebase Admin SDK private key up one level directory
cred_obj = firebase_admin.credentials.Certificate(os.path.join(os.path.dirname(os.getcwd()), 'iot-project-key.json'))
default_app = firebase_admin.initialize_app(cred_obj, {
	'databaseURL':'https://iot-project-5d1ba-default-rtdb.asia-southeast1.firebasedatabase.app/'
	})

# Data Payload Example:
# {
#     Node-1': {
#         'automation': 'on',
#         'ldr': 118,
#         'servo': 120
#     }
# }

app = Flask(__name__)

# Root path
@app.route("/", methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        ref = db.reference('/')
        # Update servo value on realtime database firebase
        # ref.child('Node-1').update({'servo': int(request.form['servoValue'])})
        if request.form['servoValue'] == 'on':
            ref.child('Node-1').update({'servo': int(0)})
            ref.child('Node-1').update({'automation': 'off'})
        elif request.form['servoValue'] == 'off':
            ref.child('Node-1').update({'servo': int(90)})
            ref.child('Node-1').update({'automation': 'off'})
        elif request.form['servoValue'] == 'auto-on':
            ref.child('Node-1').update({'automation': 'on'})
        elif request.form['servoValue'] == 'auto-off':
            ref.child('Node-1').update({'automation': 'off'})

        print('LDR: ', ref.get()['Node-1']['ldr'])
        print('Servo: ', request.form['servoValue'])

        # Render changes to web page
        return render_template(
            'index.html', servoValue=request.form['servoValue'], 
            ldrValue=db.reference('/Node-1/ldr/').get()
        )
    elif request.method == 'GET':
        # Render changes to web page
        return render_template(
            'index.html', 
            servoValue='on' if db.reference('/Node-1/servo/').get() == 0 else 'off', 
            ldrValue=db.reference('/Node-1/ldr/').get()
        )

    # Render changes to web page
    return render_template(
        'index.html', 
        servoValue='on' if db.reference('/Node-1/servo/').get() == 0 else 'off', 
        ldrValue=db.reference('/Node-1/ldr/').get()
    )

port = int(os.environ.get('PORT', 80))
if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=port)
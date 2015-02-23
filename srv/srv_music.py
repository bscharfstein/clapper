import subprocess

from flask import Flask, request, send_file

app = Flask(__name__)

@app.route('/on')
def turn_on():
    subprocess.call(['osascript', 'playpause.scpt'])
    return 'ok'

@app.route('/off')
def turn_off():
    subprocess.call(['osascript', 'playpause.scpt'])
    return 'ok'

if __name__ == '__main__':
    print 'Listening on http://localhost:80'
    app.debug = True
    import os
    # from werkzeug.wsgi import SharedDataMiddleware
    # app = SharedDataMiddleware(app, {
    #     '/': os.path.join(os.path.dirname(__file__), 'static')
    # })
    app.run(host='0.0.0.0', port=80)

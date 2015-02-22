from gevent import monkey; monkey.patch_all()
from flask import Flask, request, send_file

from socketio import socketio_manage
from socketio.namespace import BaseNamespace
from socketio.mixins import RoomsMixin, BroadcastMixin

app = Flask(__name__)

# The socket.io namespace
class PebNamespace(BaseNamespace, RoomsMixin, BroadcastMixin):
    pass

def send_message(msg):
    pkt = dict(type='event',
            name=msg,
            endpoint='')

    for sessid, socket in server.sockets.iteritems():
        print sessid
        socket.send_packet(pkt)

@app.route('/')
def index():
    return send_file('light.html')

@app.route('/on')
def turn_on():
    send_message('turn-on')
    return 'ok'

@app.route('/off')
def turn_off():
    send_message('turn-off')
    return 'ok'

@app.route("/socket.io/<path:path>")
def run_socketio(path):
    socketio_manage(request.environ, {'': PebNamespace})

if __name__ == '__main__':
    print 'Listening on http://localhost:80'
    app.debug = True
    import os
    from werkzeug.wsgi import SharedDataMiddleware
    app = SharedDataMiddleware(app, {
        '/': os.path.join(os.path.dirname(__file__), 'static')
    })
    from socketio.server import SocketIOServer
    server = SocketIOServer(('0.0.0.0', 80), app,
        namespace="socket.io", policy_server=False)
    server.serve_forever()

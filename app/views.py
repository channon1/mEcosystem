from app import app
#from pylons import request, response, session, tmpl_context, config
#from flask import *

import socket
import time

@app.route('/')
@app.route('/index')
def index():
    return render_template('index.html')

@app.route('/data')
def data():
    return Response(yunserver_sse(), mimetype='text/event-stream')


def yunserver_sse():
    try:
            soc = socket.create_connection(('localhost', 1993))
            socfile = soc.makefile()
            while True:
                line = socfile.readline()
                if not line:
                   raise StopIteration
                yield 'data: {0}\n\n'.format(line)
                time.sleep(0)
    except socket.error:
            raise StopIteration

            

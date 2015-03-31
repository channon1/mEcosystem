import sys
sys.path.append('/mnt/sda1/python-packages')

from flask import Flask

app = Flask(__name__)
from app import views

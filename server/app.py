from flask import Flask, request, jsonify, render_template
from datetime import datetime, timezone
from threading import Lock
import json, os

app = Flask(__name__)
lock = Lock()

EVENTS_FILE = os.path.join(os.path.dirname(__file__), 'events.json')
MAX_EVENTS  = 500

events = []


def load_events():
    global events
    if os.path.exists(EVENTS_FILE):
        try:
            with open(EVENTS_FILE) as f:
                events = json.load(f)
        except Exception:
            pass


def save_events():
    try:
        with open(EVENTS_FILE, 'w') as f:
            json.dump(events[-MAX_EVENTS:], f)
    except Exception:
        pass


load_events()


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/api/touch', methods=['POST'])
def receive_touch():
    body = request.get_json(force=True, silent=True) or {}
    now  = datetime.now(timezone.utc).isoformat()

    event = {
        'id':         len(events) + 1,
        'date':       body.get('date', now[:10]),
        'time':       body.get('time', now[11:19]),
        'received_at': now,
    }

    with lock:
        events.append(event)
        save_events()

    return jsonify({'ok': True, 'event': event})


@app.route('/api/events')
def get_events():
    with lock:
        return jsonify(list(reversed(events[-100:])))


@app.route('/api/clear', methods=['POST'])
def clear_events():
    with lock:
        events.clear()
        save_events()
    return jsonify({'ok': True})


if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=False)

import csv
import datetime
from flask import Flask, jsonify, redirect, render_template, request, session, url_for
import psycopg2
import logging
from flask import Flask, render_template, flash


# Flask App Set Up
app = Flask(__name__)
logging.basicConfig(filename='your_app.log', level=logging.DEBUG)
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = True
app.secret_key = 'your_secret_key'

# Database config
DB_CONFIG = {
    'dbname': 'students',
    'user': 'postgres',
    'password': '123456',
    'host': 'localhost',
}


# Endpoint to export to CSV and delete records 
@app.route('/delete', methods=['POST'])
def delete():
    try: 
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        
        # create CSV

        # Fetch attendance records from the database
        cur.execute("SELECT * FROM attendance2")
        records = cur.fetchall()
        conn.commit()

        # Convert records to a list of dictionaries
        records_dicts = [{'id': record[0], 'name': record[1],'present': record[2] } for record in records]

        # Create a timestamped CSV file with attendance records
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f'attendance_{timestamp}.csv'

        with open(filename, 'w', newline='') as csvfile:
            fieldnames = ['id', 'name', 'present']  
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(records_dicts)

        
        
        # Commit changes to the database and truncate tables
        cur.execute("TRUNCATE TABLE attendance2")
        conn.commit()
        cur.execute("TRUNCATE TABLE sensor_count")
        conn.commit()
        conn.close()
        
        return jsonify({'message': 'CVS created successfully'}), 201
    except Exception as e:
        logging.error("An error occurred in the 'index' route: %s", str(e))
        return jsonify({'error': 'An error occurred'}), 500
    

# Endpoint to get the number of present students
@app.route('/present_students_num', methods=['GET'])
def present_students_num():
    try: 
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()

        # Count the number of present students
        cur.execute("SELECT COUNT(*) FROM attendance2 WHERE present is TRUE")
        records = cur.fetchall()

        # Convert records to a list of dictionaries
        records_dicts = [{'count': record[0] } for record in records]

        conn.commit()
        conn.close()
        return records_dicts
    except Exception as e:
        logging.error("An error occurred in the 'index' route: %s", str(e))
        return jsonify({'error': 'An error occurred'}), 500
    

# Endpoint to display the list of present students
@app.route('/present', methods=['GET'])
def present():
    try: 
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()

        # Fetch records of present students from the database
        cur.execute("SELECT * FROM attendance2 WHERE present is TRUE")
        records = cur.fetchall()
        conn.commit()
        conn.close()
        # Render the records in an HTML template 
        if records:
            return render_template('students_tracker.html', records=records)
        else:
            return "<p>No studnets!</p>"
    except Exception as e:
        logging.error("An error occurred in the 'index' route: %s", str(e))
        return jsonify({'error': 'An error occurred'}), 500

# Endpoint to display the main page with student information
@app.route('/', methods=['GET'])
def index():
    try: 
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()

        # Fetch sensor count and records from attendance where present is true
        cur.execute("SELECT count FROM sensor_count WHERE ID = 1")
        sensor_count_record = cur.fetchone()
        sensor_count = sensor_count_record[0] if sensor_count_record else 0
        cur.execute("SELECT COUNT(*) FROM attendance2 WHERE present = TRUE")
        present_count_record = cur.fetchone()
        present_count = present_count_record[0] if present_count_record else 0

        # Calculate the difference between sensor count and present count
        difference = abs(sensor_count - present_count)

        # Check if the difference is greater than 10
        alert_flag = difference > 10

        # Fetch all records from attendance table
        cur.execute("SELECT * FROM attendance2 ORDER BY present DESC")
        records = cur.fetchall()

        conn.commit()
        conn.close()
        # Render the main page with student information and alert flag 
        return render_template('students_tracker.html', records=records, alert_flag=alert_flag,sensor_count=sensor_count)
        
    except Exception as e:
        logging.error("An error occurred in the 'index' route: %s", str(e))
        return jsonify({'error': 'An error occurred'}), 500


# Endpoint to update the sensor count
@app.route('/update_count', methods=['POST'])
def update_count():
    try:
        if request.headers['Content-Type'] == 'application/json':
            data = request.json
            print(data)
            cnt = int(data.get('count'))
            if cnt is None:
                return jsonify({'error': 'Missing cnt'}), 400
            conn = psycopg2.connect(**DB_CONFIG)
            cursor = conn.cursor()
            # Update sensor count in the database
            ID = 1
            insert_query =  """
                INSERT INTO sensor_count (ID, count)
                VALUES (%s,%s)
                ON CONFLICT (ID)
                DO UPDATE SET count = CASE WHEN sensor_count.count <> %s THEN %s ELSE sensor_count.count END;
            """
            cursor.execute(insert_query, (ID, cnt, cnt, cnt))
            
            conn.commit()
            conn.close()
            logging.info("Data added successfully") 
            return jsonify({'message': 'Data added successfully'}), 201
        else:
            return jsonify({'error': 'Unsupported Media Type'}), 415
    except Exception as e:
        logging.error("An error occurred in the 'update' route: %s", str(e))
        return jsonify({'error': str(e)}), 500
    

# Endpoint to update attendance based on RFID tag information
@app.route('/update', methods=['POST'])
def update():
    try:
        if request.headers['Content-Type'] == 'application/json':
            data = request.json
            uid = data.get('id')
            name = data['name'].replace('\x00', '') # Remove null characters from the name
            name = name.strip() # Strip leading and trailing whitespaces

            if uid is None:
                return jsonify({'error': 'Missing uid'}), 400
            
            conn = psycopg2.connect(**DB_CONFIG)
            cursor = conn.cursor()

            # Update attendance based on RFID tag information
            insert_query =  f"INSERT INTO attendance2 VALUES ('{uid}', '{name}', TRUE) ON CONFLICT (ID) DO UPDATE SET present = NOT attendance2.present;"
            cursor.execute(insert_query)

            conn.commit()
            conn.close()
            logging.info("Data added successfully")
            return jsonify({'message': 'Data added successfully'}), 201
        else:
            return jsonify({'error': 'Unsupported Media Type'}), 415
    except Exception as e:
        logging.error("An error occurred in the 'update' route: %s", str(e))

        return jsonify({'error': str(e)}), 500
    
# Run the Flask application
if __name__ == "__main__":
    print("Starting the Flask application...")
    app.run(host='0.0.0.0', port=80, debug=True)


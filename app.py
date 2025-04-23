import os
import requests # To make HTTP requests to the LPR API
from flask import Flask, request, jsonify

# --- Configuration ---
# Get your API token from https://platerecognizer.com/
# WARNING: Hardcoding the token like this is insecure for production/shared code.
#          Use environment variables instead if possible.
PLATE_RECOGNIZER_API_TOKEN = "23f65bf271792539d32de4df432122ca23176b01" # Your provided API token
PLATE_RECOGNIZER_API_URL = "https://api.platerecognizer.com/v1/plate-reader/"

# --- Flask App Initialization ---
app = Flask(__name__)

# --- API Endpoint for Image Upload ---
@app.route('/upload_image', methods=['POST'])
def handle_image_upload():
    print("Received image upload request...")

    if not request.data:
        print("Error: No image data received in request body.")
        return jsonify({"status": "error", "message": "No image data received"}), 400

    image_data = request.data # Raw image bytes from ESP32

    print(f"Received image size: {len(image_data)} bytes")

    # --- Call Plate Recognizer API ---
    if not PLATE_RECOGNIZER_API_TOKEN:
         print("Error: Plate Recognizer API token is missing.")
         return jsonify({"status": "error", "message": "API token not configured"}), 500

    try:
        headers = {'Authorization': f'Token {PLATE_RECOGNIZER_API_TOKEN}'}
        # Send the raw image data using 'files' parameter
        response = requests.post(
            PLATE_RECOGNIZER_API_URL,
            headers=headers,
            files={'upload': ('image.jpg', image_data, 'image/jpeg')} # Provide filename and content type
            # Optional: Add region parameter if needed, e.g., data={'regions': 'gb'} for UK
            # data={'regions': 'us-ca'} # Example: California plates
        )
        response.raise_for_status() # Raise an exception for bad status codes (4xx or 5xx)

        api_result = response.json()
        print("Plate Recognizer API Response:", api_result)

        # --- Process API Result ---
        plates = []
        if api_result.get('results'):
            plates = [res['plate'] for res in api_result['results']]
            recognized_plate = plates[0] if plates else "NOT_FOUND"
            print(f"Recognized Plate(s): {plates}")
        else:
            recognized_plate = "NOT_FOUND"
            print("No plates found in the image.")

        # --- Respond to ESP32 ---
        # You can add more logic here (database lookup, gate control command, etc.)
        return jsonify({
            "status": "success",
            "plate": recognized_plate, # Send back the first plate found
            "all_plates": plates,      # Send all plates found
            "raw_response": api_result # Send the full API response for debugging
            }), 200

    except requests.exceptions.RequestException as e:
        print(f"Error calling Plate Recognizer API: {e}")
        # Check if the response object exists and has content before trying to read it
        error_details = "No response body"
        if e.response is not None:
            try:
                 error_details = e.response.text # Get API error message if available
                 print(f"API Error Response Body: {error_details}")
            except Exception:
                 error_details = "Could not read error response body."

        return jsonify({
            "status": "error",
            "message": f"Failed to process image with LPR API: {str(e)}",
            "api_error_details": error_details
            }), 500
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return jsonify({"status": "error", "message": f"An internal server error occurred: {str(e)}"}), 500


# --- Run the Flask App ---
# Use host='0.0.0.0' to make it accessible on your network
if __name__ == '__main__':
    # For local testing, run on HTTP.
    # When deploying (e.g. Render), Gunicorn/Uvicorn will likely handle this.
    # Render automatically uses port 10000 for free web services.
    port = int(os.environ.get('PORT', 5000)) # Use PORT env var if available, else default to 5000
    app.run(host='0.0.0.0', port=port, debug=True) # Enable debug for development
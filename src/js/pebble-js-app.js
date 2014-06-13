'use strict';

/**
 * Fetches location data
 * @param  {Object} pos GPS and other info from Pebble
 * @return {Void}
 */
function fetch_location_data(pos) {

  // Parameters
  var version = Date.now(),
      latitude = pos.coords.latitude,
      longitude = pos.coords.longitude;

  // Endpoint URL
  var url = 'http://coffee.bringitfast.com/api/v1/search?' +
          '&client_secret=HjuCrMRN' +
          '&lat=' + latitude +
          '&lon=' + longitude +
          '&radius=3' +
          '&term=coffee' +
          '&v=' + version;

  /**
   * Executes HTTP Request
   * @type {XMLHttpRequest}
   */
  var xhr = new XMLHttpRequest();
  xhr.timeout = 6000;
  xhr.open('POST', url, true);

  /**
   * XHR onload method, gets called once we receive a response from server
   * @param  {Object} e   The XHR event
   * @return {Void}
   */
  xhr.onload = function(e) {

    var location = '',
        business = '';

    if (xhr.readyState == 4 && xhr.status == 200) {

      var response = JSON.parse(xhr.responseText);
      var venue = response.data[0];

      business = venue.name;
      location = venue.address + ', ' + venue.city;

    } else {
      business = 'No coffee shops :(';
    }

    // Sends message back to Pebble device
    Pebble.sendAppMessage({
      location: location,
      business: business
    });
  };

  xhr.send(null);
}

/**
 * Executes when there was an error fetching location
 * @param  {Object} err Error Object
 * @return {Void}
 */
function fetch_location_error(err) {
  Pebble.sendAppMessage({location: 'Unable to retrieve location'});
}


/**
 * Gets run when pebble device is ready
 */
var locationOptions = { timeout: 15000, maximumAge: 60000 };
Pebble.addEventListener('ready', function(e) {
  var locationWatcher = window.navigator.geolocation.watchPosition(fetch_location_data, fetch_location_error, locationOptions);
});

'use strict';
/* global Pebble */

/**
 * XHR Request
 * @type {Object}
 */
var xhr;

/**
 * Determines if a location has been previously found
 * @type {Boolean}
 */
var hasPreviousLocation = false;

/**
 * Fetches location data
 * @param  {Object} pos GPS and other info from Pebble
 * @return {Void}
 */
function fetch_location_data(pos) {

  // Parameters
  var latitude  = pos.coords.latitude,
      longitude = pos.coords.longitude;

  // Endpoint URL
  var url = 'http://coffee.bringitfast.com/api/v1/search?' +
          '&client_secret=HjuCrMRN' +
          '&limit=1' +
          '&radius=3' +
          '&term=coffee' +
          '&lat=' + latitude +
          '&lon=' + longitude;

  /**
   * Executes HTTP Request
   * @type {XMLHttpRequest}
   */
  xhr = new XMLHttpRequest();
  xhr.timeout = 6000;
  xhr.open('POST', url, true);
  xhr.onload = onSuccess;
  xhr.send(null);
}

/**
 * XHR onload method, gets called once we receive a response from server
 * @param  {Object} e   The XHR event
 * @return {Void}
 */
function onSuccess() {

  var location = '',
      business = '',
      response,
      venue;

  if (xhr.readyState === 4 && xhr.status === 200) {

    response = JSON.parse(xhr.responseText);
    venue = response.data[0];

    // Trims if name is too long
    if (venue.name.length >= 28) {
      venue.name = trimWords(venue.name);
    }

    location = '';
    business = venue.name + '\n' + venue.address + ', ' + venue.city;

  } else {
    business = 'No coffee shops :(';
  }

  // Sets flag to true
  hasPreviousLocation = true;

  // Sends message back to Pebble device
  Pebble.sendAppMessage({
    location: location,
    business: business
  });
}

/**
 * Executes when there was an error fetching location
 * @param  {Object} err Error Object
 * @return {Void}
 */
function fetch_location_error() {

  if (!hasPreviousLocation) {
    return;
  }

  // only sends message if no previous location was found
  Pebble.sendAppMessage({
    location: '',
    business: 'Location unavailable'
  });

}

/**
 * Trims words
 * @param  {String} word
 * @return {String}
 */
function trimWords(word) {
  return word.replace(/^(.{20}[^\s]*).*/, '$1') + '...';
}


/**
 * Gets run when pebble device is ready
 * 5 minutes timeout and max age
 */
var locationOptions = {
  timeout: 300000,
  maximumAge: 300000
};

Pebble.addEventListener('ready', function onPebbleReady() {
  window.navigator.geolocation.watchPosition(
    fetch_location_data,
    fetch_location_error,
    locationOptions
  );
});

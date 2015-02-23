Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log(e.payload['0']);
    var xmlhttp = new XMLHttpRequest();
    if (e.payload['0'])
      xmlhttp.open("GET", "http://clapper.kirkland.house/on", false);
    else
      xmlhttp.open("GET", "http://clapper.kirkland.house/off", false);
    xmlhttp.send();
  }
);
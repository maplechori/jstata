const ref = require('ref');
const ffi = require('ffi');

libstata = ffi.Library(__dirname + '/stata.so', {
      'js_stata_read' : ['string', ['string']],
      'js_stata_write' : ['void', ['string', 'string']],
      });


module.exports = {

  stataRead: function(filename) {
    return JSON.parse(libstata.js_stata_read(filename));
  },

  stataWrite: function(filename, obj) {
    return libstata.js_stata_write(filename, JSON.stringify(obj));
  }
};


var ref = require('ref');
var ffi = require('ffi');
var Struct = require('ref-struct');

libstata = ffi.Library('stata/libstata', {
      'js_stata_open' : ['string', ['string']]
});


module.exports = {

  stataRead: function(filename) {


    return JSON.parse(libstata.js_stata_open(filename));
  },

  stataWrite: function() {
    return { write: 'bleh'}
  }


}

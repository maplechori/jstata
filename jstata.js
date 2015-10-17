var ref = require('ref');
var ffi = require('ffi');
var Struct = require('ref-struct');


var libstata = ffi.Library('stata/libstata', {
      'js_stata_open' : ['pointer', ['string']]
});


console.log(libstata.js_stata_open('stata/filename4.dta'));


module.exports = {

  stataRead: function() {
    return { data: 'blah' }
  },

  stataWrite: function() {
    return { write: 'bleh'}
  }


}

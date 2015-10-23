
var jstata = require('./jstata.js');


var exportData = {
  data: [
    ['hi'],
    ['wee']
  ],
  metadata: {
      timestamp: '12 Dec 2015 13:00',
      observations: 2,
      variables: 1,
      datalabel: 'Created by Adrian',
      version: -12,
      nlabels: 1,
  },
  variables: [
    {
      vfmt: '%3s',
      name: 'message',
      dlabels: 'message variable',
      vlabels: '',
      valueType: 3
    }
  ],
  labels:{

  }

}



var o = jstata.stataRead('stata/filename4.dta');
console.log(libstata);
console.log(o);
console.log(exportData);
jstata.stataWrite('stata/export.dta', exportData);

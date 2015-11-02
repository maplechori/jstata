
var jstata = require('./jstata.js');

var exportData = {
  data: [
    ['hi',1],
    ['wee',2]
  ],
  metadata: {
      timestamp: '21 Oct 2015 23:26',
      observations: 2,
      variables: 2,
      datalabel: 'JStata',
      version: 10,
      nlabels: 1,
  },
  variables: [
    {
      vfmt: '%3s',
      name: 'message',
      dlabels: 'message variable',
      vlabels: '',
      valueType: 3
    },
    {
      vfmt: '%9.0g',
      name: 'number',
      dlabels: 'number variable',
      vlabels: 'TYesNo',
      valueType: 253
    }
  ],
  labels:{
    'TYesNo': [
        [1, 'Yes'],
        [2, 'No' ]
      ]
  }

}; 



jstata.stataWrite('export.dta', exportData);

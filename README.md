# jstata
NodeJS module to read and write Stata files

*It depends on libjansson-dev. A static copy is available as part of the repository.*

**You can also install it directly using npm.**

```node
var js = require("jstata");
var stata_file = js.stataRead('filename.dta');
console.log(stata_file);

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


js.stataWrite('export.dta', exportData);

```

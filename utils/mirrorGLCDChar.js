string = "00000000,00000000,00000000,00000000,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00000001,11111110,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00011111,11111110,00011111,00000000,00000000,00000000,00000000";

const COLUMNS = 2;

let resultStr = '';
let buffer = [['']];
let columnsCounter = 0;
let row = 0;

fillTheBuffer();
mirrorAllBytes();
prependBinarySign();
printColumnsLastRowsFirst();
//printRowsFirstColumnsLast();

function fillTheBuffer() {
  for(let i = 0; i < string.length; i++) {
    if (string[i] === ',') {
      if (columnsCounter === COLUMNS - 1) {
        columnsCounter = 0;
        row++;
        buffer[row] = [[]];
      } else {
        columnsCounter++;
        buffer[row][columnsCounter] = '';
      }
    } else {
      buffer[row][columnsCounter] += string[i];
    }
  }
}

function mirrorAllBytes() {
  [...Array(row)].forEach((_, i) => {
    [...Array(COLUMNS)].forEach((_, j) => {
      buffer[i][j] = mirrorByte(buffer[i][j]);
    });
  });
}

function prependBinarySign() {
  [...Array(row + 1)].forEach((_, i) => {
    [...Array(COLUMNS)].forEach((_, j) => {
      buffer[i][j] = '0b' + buffer[i][j];
    });
  });
}

function printColumnsLastRowsFirst() {
  
    for (let j = COLUMNS - 1; j >= 0; j--) {
      [...Array(row)].forEach((_, i) => {
        resultStr += buffer[i][j]  + (i === row - 1 ? '' : ',');
        resultStr += '\n';
      });
      resultStr += '\n';
    }
    
}

function printRowsFirstColumnsLast() {
  
  [...Array(row + 1)].forEach((_, i) => {
    [...Array(COLUMNS)].forEach((_, j) => {
      resultStr += buffer[i][j]  + (i === row && j === COLUMNS - 1 ? '' : ',');
      resultStr += '\n';
    });
  });
  
}

function mirrorByte(byteStr) {
  let buffer = '';
  for (let i = 7; i >= 0; i--) {
    buffer += byteStr[i];
  }
  return buffer;
}
console.log(resultStr);
return resultStr;
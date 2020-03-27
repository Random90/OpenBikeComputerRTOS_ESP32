string = "00000000,00000000,00000000,00000000,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00000001,11111110,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00011111,11111110,00011111,00000000,00000000,00000000,00000000";

const COLUMNS = 2;

let buffer = [['']];
let columnsCounter = 0;
let row = 0;

fillTheBuffer();
mirrorAllBytes();
parseColumnsLastRowsFirst();
rotateBmpTopRight90();
prependBinarySign();
const result = asString();
console.log(result);
return result;


function fillTheBuffer() {
  for (let i = 0; i < string.length; i++) {
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
  buffer = buffer.map((value) => '0b' + value);
}

function parseColumnsLastRowsFirst() {
  let newBuffer = [];
  for (let j = COLUMNS - 1; j >= 0; j--) {
    [...Array(row)].forEach((_, i) => {
      newBuffer.push(buffer[i][j]);
    });
  }
  buffer = newBuffer;
}

function mirrorByte(byteStr) {
  let buffer = '';
  for (let i = 7; i >= 0; i--) {
    buffer += byteStr[i];
  }
  return buffer;
}

// use bitwise operators?
function rotateBmpTopRight90() {
  let rotatedBuffer = [];
  let rotatedRowIdx = 0;
  for (let bitIdx = 7; bitIdx >= 0; bitIdx--) {
    buffer.forEach((byteStr, rowIdx) => {
      if (!rotatedBuffer[rotatedRowIdx]) {
        rotatedBuffer[rotatedRowIdx] = '';
      }
      // TODO calculate rotated size and append empty bytes?
      rotatedBuffer[rotatedRowIdx] += byteStr[bitIdx];
      if (rotatedBuffer[rotatedRowIdx].length === 8) {
        rotatedRowIdx++;
      }
    })
  }

  buffer = rotatedBuffer;
}

function asString() {
  let resultStr = '';
  buffer.forEach((byte, i) => {
    resultStr += byte + (i === buffer.length - 1 ? ' ' : ',\n');
  })
  return resultStr;
}
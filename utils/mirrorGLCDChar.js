string = "01100000,00011000,00111000,01000000,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00000001,11111110,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00011111,11111110,00011111,00000000,00000000,00000000,00000000";

const LINES = 2;

let string2 = '';
let buffer = '';
let linesCounter = 0;
for(let i = 0; i < string.length; i++) {
  buffer += string[i];
  if (string[i] === ',' || i === string.length - 1) {
    linesCounter++;
    string2 += 
      mirrorByte(buffer) + 
      (i === string.length - 1  ? '' : ',');

    if (linesCounter === LINES) {
      linesCounter = 0;
      string2 += "\n";
    }
    buffer = '';
    
  }
}

function mirrorByte(byteStr) {
  let buffer = '';
  for (let i = 7; i >= 0; i--) {
    buffer += byteStr[i];
  }
  return buffer;
}
console.log(string2);
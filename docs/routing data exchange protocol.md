# Routing Data Exchange Protocol

## Byte 0

Number of routes included.

## Byte 1+

### Route format

Byte 0: Target machine.  
Byte 1: Cost

Cost should not include the cost of sending it to the sender of this package (so recipient have to add 1).

## Padding

If payload is not divisible by 4, add 1 byte at a time until it is.

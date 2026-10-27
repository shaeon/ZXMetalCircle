# Genera el menú DISK de Mar Océana adaptado a ZXMetalCircle (sin las opciones del ZX-Uno)
import struct, re, math
T=["SPECTRUM","PLAY","RND","INKEY$","PI","FN","POINT","SCREEN$","ATTR","AT","TAB","VAL$","CODE","VAL","LEN","SIN","COS","TAN","ASN","ACS","ATN","LN","EXP","INT","SQR","SGN","ABS","PEEK","IN","USR","STR$","CHR$","NOT","BIN","OR","AND","<=",">=","<>","LINE","THEN","TO","STEP","DEF FN","CAT","FORMAT","MOVE","ERASE","OPEN #","CLOSE #","MERGE","VERIFY","BEEP","CIRCLE","INK","PAPER","FLASH","BRIGHT","INVERSE","OVER","OUT","LPRINT","LLIST","STOP","READ","DATA","RESTORE","NEW","BORDER","CONTINUE","DIM","REM","FOR","GO TO","GO SUB","INPUT","LOAD","LIST","LET","PAUSE","NEXT","POKE","PRINT","PLOT","RUN","SAVE","RANDOMIZE","IF","CLS","DRAW","CLEAR","RETURN","COPY"]
KW=sorted([(t,0xA3+i) for i,t in enumerate(T) if t[0].isalpha()],key=lambda x:-len(x[0]))
def num5(v):
    if v==int(v) and -65535<=v<=65535:
        n=int(v); s=0
        if n<0: s=0xFF; n=65536+n
        return bytes([0,s,n&255,n>>8,0])
    m,e=math.frexp(abs(v)); mant=int(round(m*2**32)) & 0xFFFFFFFF
    if v>0: mant&=0x7FFFFFFF
    return bytes([e+128])+struct.pack('>I',mant)
def tok(src):
    out=bytearray(); i=0
    while i<len(src):
        c=src[i]
        if c=='"':
            j=src.index('"',i+1); out+=src[i:j+1].encode('latin1'); i=j+1; continue
        if c.isdigit() or (c=='.' and i+1<len(src) and src[i+1].isdigit()):
            m=re.match(r'\d*\.?\d+',src[i:]); t=m.group(0); out+=t.encode()+b'\x0e'+num5(float(t)); i+=len(t); continue
        if c.isalpha():
            for k,code in KW:
                if src[i:i+len(k)].upper()==k and not (k[-1].isalpha() and i+len(k)<len(src) and src[i+len(k)].isalpha() and k not in ('AT','TO','OR','IN','FN')):
                    out.append(code); i+=len(k)
                    while i<len(src) and src[i]==' ': i+=1
                    break
            else:
                out+=c.encode(); i+=1
            continue
        for op,code in (('<=',0xC7),('>=',0xC8),('<>',0xC9)):
            if src.startswith(op,i): out.append(code); i+=2; break
        else:
            if c!=' ' or (out and out[-1]<0xA3 and False): pass
            if c!=' ': out+=c.encode()
            i+=1
    return bytes(out)+b'\r'
# Programa original (líneas con gráficos definidos por el usuario se reutilizan tal cual)
d=open('/tmp/mega/sis/disk','rb').read(); L=d[16]|d[17]<<8; p=d[128:128+L]; orig={}; i=0
while i+4<=len(p):
    ln=p[i]<<8|p[i+1]; ll=p[i+2]|p[i+3]<<8
    if ln>9999: break
    orig[ln]=p[i+4:i+4+ll]; i+=4+ll
line80=orig[80].replace(b'ZX-Uno +3e ',b'ZXMetal +3e')
assert line80!=orig[80]
P='PRINT BRIGHT 1; PAPER 7; INK 0; AT '
H='PRINT OVER 1; BRIGHT 1; PAPER 7; INK 0; AT '
lines=[
 (10,tok('BORDER 1: PAPER 1: CLEAR 63099')),
 (20,orig[20]),(30,orig[30]),(40,orig[40]),(50,orig[50]),(60,orig[60]),(70,orig[70]),(75,orig[75]),
 (80,line80),(90,orig[90]),(100,orig[100]),(110,orig[110]),
 (120,tok(P+'10,7;" WB +3e Setup    "')),
 (130,tok(P+'11,7;" Exit            "')),
 (140,tok(P+'12,7;"                 "')),
 (190,tok('PLOT 56,119: DRAW 0,-47: DRAW 135,0: DRAW 0,47')),
 (200,orig[200]),
 (210,tok('FOR n=8 TO 12: '+H+'n,7;"                 ": NEXT n')),
 (220,tok('LET L=1: LET M=0')),
 (230,orig[230]),
 (240,tok('IF A$=CHR$ (11) AND L=1 THEN LET L=5: '+H+'7,7;"                 ": LET M=1: GO TO 280')),
 (250,tok('IF A$=CHR$ (10) AND L=5 THEN LET L=1: '+H+'11,7;"                 ": LET M=1: GO TO 280')),
 (260,orig[260]),
 (270,tok('IF A$=CHR$ (10) AND L<5 THEN LET L=L+1: '+H+'L+5,7;"                 ": LET M=1')),
 (280,orig[280]),(290,orig[290]),(300,orig[300]),(310,orig[310]),(320,orig[320]),
 (330,tok('IF L=2 THEN MOVE "A:" OUT : MOVE "B:" OUT : MOVE "C:" OUT : MOVE "A:" IN "CP/M": MOVE "B:" IN "SOFTWARE": LOAD "A:": LOAD "CPM.BAS"')),
 (340,orig[340]),
 (350,tok('IF L=4 THEN LOAD "SETUP"')),
 (360,tok('IF L=5 THEN NEW')),
]
prog=b''.join(struct.pack('>H',n)+struct.pack('<H',len(b))+b for n,b in lines)
open('DISK.bas.raw','wb').write(prog)
# +3DOS: cabecera de 128 bytes, autoarranque en la línea 1 (como el original)
hdr=bytearray(128); hdr[0:8]=b'PLUS3DOS'; hdr[8]=0x1A; hdr[9]=1; hdr[10]=0
struct.pack_into('<I',hdr,11,128+len(prog)); hdr[15]=0; struct.pack_into('<HHH',hdr,16,len(prog),1,len(prog))
hdr[127]=sum(hdr[:127])&255
open('DISK','wb').write(bytes(hdr)+prog)
# también en una cinta .tap (programa "DISK", autoarranque en la línea 1)
def blk(flag,data):
    b=bytes([flag])+data; x=0
    for c in b: x^=c
    b+=bytes([x]); return struct.pack('<H',len(b))+b
h=bytes([0])+b'DISK      '+struct.pack('<HHH',len(prog),1,len(prog))
open('menu_zxmetal.tap','wb').write(blk(0,h)+blk(0xFF,prog))
print('programa', len(prog), 'bytes')

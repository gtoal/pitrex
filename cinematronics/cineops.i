/* ---------------------------------------------- Main jump table cineops.c */

/* cineops.c is meant to be included directly into cinecpu.c, or turned
 * into a master dispatcher macro, or some other horrible thing. cineops.c
 * is the RCstate dispatcher, which also causes state changes. This design
 * keeps things running very fast, since no costly flag calculations
 * need to be performed. Thank Zonn for this twisted but effective
 * idea.
 */

/* table for RCstate "A" -- Use this table if the last opcode was not
 * an ACC related opcode, and was not a B flip/flop operation.
 * Translation:
 *   Any ACC related routine will use A-reg and go on to opCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions remain in opCodeTblA
 *   JMI will use the current sign of the A-reg
 */
 opCodeTblA:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_A_AA;
   case 1:
      goto opLDAimm_A_AA;
   case 2:
      goto opLDAimm_A_AA;
   case 3:
      goto opLDAimm_A_AA;
   case 4:
      goto opLDAimm_A_AA;
   case 5:
      goto opLDAimm_A_AA;
   case 6:
      goto opLDAimm_A_AA;
   case 7:
      goto opLDAimm_A_AA;
   case 8:
      goto opLDAimm_A_AA;
   case 9:
      goto opLDAimm_A_AA;
   case 10:
      goto opLDAimm_A_AA;
   case 11:
      goto opLDAimm_A_AA;
   case 12:
      goto opLDAimm_A_AA;
   case 13:
      goto opLDAimm_A_AA;
   case 14:
      goto opLDAimm_A_AA;
   case 15:
      goto opLDAimm_A_AA;
   case 16:
      goto opINP_A_AA;
   case 17:
      goto opINP_A_AA;
   case 18:
      goto opINP_A_AA;
   case 19:
      goto opINP_A_AA;
   case 20:
      goto opINP_A_AA;
   case 21:
      goto opINP_A_AA;
   case 22:
      goto opINP_A_AA;
   case 23:
      goto opINP_A_AA;
   case 24:
      goto opINP_A_AA;
   case 25:
      goto opINP_A_AA;
   case 26:
      goto opINP_A_AA;
   case 27:
      goto opINP_A_AA;
   case 28:
      goto opINP_A_AA;
   case 29:
      goto opINP_A_AA;
   case 30:
      goto opINP_A_AA;
   case 31:
      goto opINP_A_AA;
   case 32:
      goto opADDimmX_A_AA;
   case 33:
      goto opADDimm_A_AA;
   case 34:
      goto opADDimm_A_AA;
   case 35:
      goto opADDimm_A_AA;
   case 36:
      goto opADDimm_A_AA;
   case 37:
      goto opADDimm_A_AA;
   case 38:
      goto opADDimm_A_AA;
   case 39:
      goto opADDimm_A_AA;
   case 40:
      goto opADDimm_A_AA;
   case 41:
      goto opADDimm_A_AA;
   case 42:
      goto opADDimm_A_AA;
   case 43:
      goto opADDimm_A_AA;
   case 44:
      goto opADDimm_A_AA;
   case 45:
      goto opADDimm_A_AA;
   case 46:
      goto opADDimm_A_AA;
   case 47:
      goto opADDimm_A_AA;
   case 48:
      goto opSUBimmX_A_AA;
   case 49:
      goto opSUBimm_A_AA;
   case 50:
      goto opSUBimm_A_AA;
   case 51:
      goto opSUBimm_A_AA;
   case 52:
      goto opSUBimm_A_AA;
   case 53:
      goto opSUBimm_A_AA;
   case 54:
      goto opSUBimm_A_AA;
   case 55:
      goto opSUBimm_A_AA;
   case 56:
      goto opSUBimm_A_AA;
   case 57:
      goto opSUBimm_A_AA;
   case 58:
      goto opSUBimm_A_AA;
   case 59:
      goto opSUBimm_A_AA;
   case 60:
      goto opSUBimm_A_AA;
   case 61:
      goto opSUBimm_A_AA;
   case 62:
      goto opSUBimm_A_AA;
   case 63:
      goto opSUBimm_A_AA;
   case 64:
      goto opLDJimm_A_A;
   case 65:
      goto opLDJimm_A_A;
   case 66:
      goto opLDJimm_A_A;
   case 67:
      goto opLDJimm_A_A;
   case 68:
      goto opLDJimm_A_A;
   case 69:
      goto opLDJimm_A_A;
   case 70:
      goto opLDJimm_A_A;
   case 71:
      goto opLDJimm_A_A;
   case 72:
      goto opLDJimm_A_A;
   case 73:
      goto opLDJimm_A_A;
   case 74:
      goto opLDJimm_A_A;
   case 75:
      goto opLDJimm_A_A;
   case 76:
      goto opLDJimm_A_A;
   case 77:
      goto opLDJimm_A_A;
   case 78:
      goto opLDJimm_A_A;
   case 79:
      goto opLDJimm_A_A;
   case 80:
      goto tJPP_A_B;		/* redirector */
   case 81:
      goto tJMI_A_B;		/* redirector */
   case 82:
      goto opJDR_A_B;
   case 83:
      goto opJLT_A_B;
   case 84:
      goto opJEQ_A_B;
   case 85:
      goto opJNC_A_B;
   case 86:
      goto opJA0_A_B;
   case 87:
      goto opNOP_A_B;
   case 88:
      goto opJMP_A_A;
   case 89:
      goto tJMI_A_A;		/* redirector */
   case 90:
      goto opJDR_A_A;
   case 91:
      goto opJLT_A_A;
   case 92:
      goto opJEQ_A_A;
   case 93:
      goto opJNC_A_A;
   case 94:
      goto opJA0_A_A;
   case 95:
      goto opNOP_A_A;
   case 96:
      goto opADDdir_A_AA;
   case 97:
      goto opADDdir_A_AA;
   case 98:
      goto opADDdir_A_AA;
   case 99:
      goto opADDdir_A_AA;
   case 100:
      goto opADDdir_A_AA;
   case 101:
      goto opADDdir_A_AA;
   case 102:
      goto opADDdir_A_AA;
   case 103:
      goto opADDdir_A_AA;
   case 104:
      goto opADDdir_A_AA;
   case 105:
      goto opADDdir_A_AA;
   case 106:
      goto opADDdir_A_AA;
   case 107:
      goto opADDdir_A_AA;
   case 108:
      goto opADDdir_A_AA;
   case 109:
      goto opADDdir_A_AA;
   case 110:
      goto opADDdir_A_AA;
   case 111:
      goto opADDdir_A_AA;
   case 112:
      goto opSUBdir_A_AA;
   case 113:
      goto opSUBdir_A_AA;
   case 114:
      goto opSUBdir_A_AA;
   case 115:
      goto opSUBdir_A_AA;
   case 116:
      goto opSUBdir_A_AA;
   case 117:
      goto opSUBdir_A_AA;
   case 118:
      goto opSUBdir_A_AA;
   case 119:
      goto opSUBdir_A_AA;
   case 120:
      goto opSUBdir_A_AA;
   case 121:
      goto opSUBdir_A_AA;
   case 122:
      goto opSUBdir_A_AA;
   case 123:
      goto opSUBdir_A_AA;
   case 124:
      goto opSUBdir_A_AA;
   case 125:
      goto opSUBdir_A_AA;
   case 126:
      goto opSUBdir_A_AA;
   case 127:
      goto opSUBdir_A_AA;
   case 128:
      goto opLDPimm_A_A;
   case 129:
      goto opLDPimm_A_A;
   case 130:
      goto opLDPimm_A_A;
   case 131:
      goto opLDPimm_A_A;
   case 132:
      goto opLDPimm_A_A;
   case 133:
      goto opLDPimm_A_A;
   case 134:
      goto opLDPimm_A_A;
   case 135:
      goto opLDPimm_A_A;
   case 136:
      goto opLDPimm_A_A;
   case 137:
      goto opLDPimm_A_A;
   case 138:
      goto opLDPimm_A_A;
   case 139:
      goto opLDPimm_A_A;
   case 140:
      goto opLDPimm_A_A;
   case 141:
      goto opLDPimm_A_A;
   case 142:
      goto opLDPimm_A_A;
   case 143:
      goto opLDPimm_A_A;
   case 144:
      goto tOUT_A_A;		/* redirector */
   case 145:
      goto tOUT_A_A;		/* redirector */
   case 146:
      goto tOUT_A_A;		/* redirector */
   case 147:
      goto tOUT_A_A;		/* redirector */
   case 148:
      goto tOUT_A_A;		/* redirector */
   case 149:
      goto tOUT_A_A;		/* redirector */
   case 150:
      goto tOUT_A_A;		/* redirector */
   case 151:
      goto tOUT_A_A;		/* redirector */
   case 152:
      goto tOUT_A_A;		/* redirector */
   case 153:
      goto tOUT_A_A;		/* redirector */
   case 154:
      goto tOUT_A_A;		/* redirector */
   case 155:
      goto tOUT_A_A;		/* redirector */
   case 156:
      goto tOUT_A_A;		/* redirector */
   case 157:
      goto tOUT_A_A;		/* redirector */
   case 158:
      goto tOUT_A_A;		/* redirector */
   case 159:
      goto tOUT_A_A;		/* redirector */
   case 160:
      goto opLDAdir_A_AA;
   case 161:
      goto opLDAdir_A_AA;
   case 162:
      goto opLDAdir_A_AA;
   case 163:
      goto opLDAdir_A_AA;
   case 164:
      goto opLDAdir_A_AA;
   case 165:
      goto opLDAdir_A_AA;
   case 166:
      goto opLDAdir_A_AA;
   case 167:
      goto opLDAdir_A_AA;
   case 168:
      goto opLDAdir_A_AA;
   case 169:
      goto opLDAdir_A_AA;
   case 170:
      goto opLDAdir_A_AA;
   case 171:
      goto opLDAdir_A_AA;
   case 172:
      goto opLDAdir_A_AA;
   case 173:
      goto opLDAdir_A_AA;
   case 174:
      goto opLDAdir_A_AA;
   case 175:
      goto opLDAdir_A_AA;
   case 176:
      goto opCMPdir_A_AA;
   case 177:
      goto opCMPdir_A_AA;
   case 178:
      goto opCMPdir_A_AA;
   case 179:
      goto opCMPdir_A_AA;
   case 180:
      goto opCMPdir_A_AA;
   case 181:
      goto opCMPdir_A_AA;
   case 182:
      goto opCMPdir_A_AA;
   case 183:
      goto opCMPdir_A_AA;
   case 184:
      goto opCMPdir_A_AA;
   case 185:
      goto opCMPdir_A_AA;
   case 186:
      goto opCMPdir_A_AA;
   case 187:
      goto opCMPdir_A_AA;
   case 188:
      goto opCMPdir_A_AA;
   case 189:
      goto opCMPdir_A_AA;
   case 190:
      goto opCMPdir_A_AA;
   case 191:
      goto opCMPdir_A_AA;
   case 192:
      goto opLDIdir_A_A;
   case 193:
      goto opLDIdir_A_A;
   case 194:
      goto opLDIdir_A_A;
   case 195:
      goto opLDIdir_A_A;
   case 196:
      goto opLDIdir_A_A;
   case 197:
      goto opLDIdir_A_A;
   case 198:
      goto opLDIdir_A_A;
   case 199:
      goto opLDIdir_A_A;
   case 200:
      goto opLDIdir_A_A;
   case 201:
      goto opLDIdir_A_A;
   case 202:
      goto opLDIdir_A_A;
   case 203:
      goto opLDIdir_A_A;
   case 204:
      goto opLDIdir_A_A;
   case 205:
      goto opLDIdir_A_A;
   case 206:
      goto opLDIdir_A_A;
   case 207:
      goto opLDIdir_A_A;
   case 208:
      goto opSTAdir_A_A;
   case 209:
      goto opSTAdir_A_A;
   case 210:
      goto opSTAdir_A_A;
   case 211:
      goto opSTAdir_A_A;
   case 212:
      goto opSTAdir_A_A;
   case 213:
      goto opSTAdir_A_A;
   case 214:
      goto opSTAdir_A_A;
   case 215:
      goto opSTAdir_A_A;
   case 216:
      goto opSTAdir_A_A;
   case 217:
      goto opSTAdir_A_A;
   case 218:
      goto opSTAdir_A_A;
   case 219:
      goto opSTAdir_A_A;
   case 220:
      goto opSTAdir_A_A;
   case 221:
      goto opSTAdir_A_A;
   case 222:
      goto opSTAdir_A_A;
   case 223:
      goto opSTAdir_A_A;
   case 224:
      goto opVDR_A_A;
   case 225:
      goto opLDJirg_A_A;
   case 226:
      goto opXLT_A_AA;
   case 227:
      goto opMULirg_A_AA;
   case 228:
      goto opLLT_A_AA;
   case 229:
      goto opWAI_A_A;
   case 230:
      goto opSTAirg_A_A;
   case 231:
      goto opADDirg_A_AA;
   case 232:
      goto opSUBirg_A_AA;
   case 233:
      goto opANDirg_A_AA;
   case 234:
      goto opLDAirg_A_AA;
   case 235:
      goto opLSRe_A_AA;
   case 236:
      goto opLSLe_A_AA;
   case 237:
      goto opASRe_A_AA;
   case 238:
      goto opASRDe_A_AA;
   case 239:
      goto opLSLDe_A_AA;
   case 240:
      goto opVIN_A_A;
   case 241:
      goto opLDJirg_A_A;
   case 242:
      goto opXLT_A_AA;
   case 243:
      goto opMULirg_A_AA;
   case 244:
      goto opLLT_A_AA;
   case 245:
      goto opWAI_A_A;
   case 246:
      goto opSTAirg_A_A;
   case 247:
      goto opAWDirg_A_AA;
   case 248:
      goto opSUBirg_A_AA;
   case 249:
      goto opANDirg_A_AA;
   case 250:
      goto opLDAirg_A_AA;
   case 251:
      goto opLSRf_A_AA;
   case 252:
      goto opLSLf_A_AA;
   case 253:
      goto opASRf_A_AA;
   case 254:
      goto opASRDf_A_AA;
   case 255:
      goto opLSLDf_A_AA;
   } /* switch on opcode */

/* opcode table AA -- Use this table if the last opcode was an ACC
 * related opcode. Translation:
 *   Any ACC related routine will use A-reg and remain in OpCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions will jump to opCodeTblA
 *   JMI will use the sign of acc_old
 */

 opCodeTblAA:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_AA_AA;
   case 1:
      goto opLDAimm_AA_AA;
   case 2:
      goto opLDAimm_AA_AA;
   case 3:
      goto opLDAimm_AA_AA;
   case 4:
      goto opLDAimm_AA_AA;
   case 5:
      goto opLDAimm_AA_AA;
   case 6:
      goto opLDAimm_AA_AA;
   case 7:
      goto opLDAimm_AA_AA;
   case 8:
      goto opLDAimm_AA_AA;
   case 9:
      goto opLDAimm_AA_AA;
   case 10:
      goto opLDAimm_AA_AA;
   case 11:
      goto opLDAimm_AA_AA;
   case 12:
      goto opLDAimm_AA_AA;
   case 13:
      goto opLDAimm_AA_AA;
   case 14:
      goto opLDAimm_AA_AA;
   case 15:
      goto opLDAimm_AA_AA;
   case 16:
      goto opINP_AA_AA;
   case 17:
      goto opINP_AA_AA;
   case 18:
      goto opINP_AA_AA;
   case 19:
      goto opINP_AA_AA;
   case 20:
      goto opINP_AA_AA;
   case 21:
      goto opINP_AA_AA;
   case 22:
      goto opINP_AA_AA;
   case 23:
      goto opINP_AA_AA;
   case 24:
      goto opINP_AA_AA;
   case 25:
      goto opINP_AA_AA;
   case 26:
      goto opINP_AA_AA;
   case 27:
      goto opINP_AA_AA;
   case 28:
      goto opINP_AA_AA;
   case 29:
      goto opINP_AA_AA;
   case 30:
      goto opINP_AA_AA;
   case 31:
      goto opINP_AA_AA;
   case 32:
      goto opADDimmX_AA_AA;
   case 33:
      goto opADDimm_AA_AA;
   case 34:
      goto opADDimm_AA_AA;
   case 35:
      goto opADDimm_AA_AA;
   case 36:
      goto opADDimm_AA_AA;
   case 37:
      goto opADDimm_AA_AA;
   case 38:
      goto opADDimm_AA_AA;
   case 39:
      goto opADDimm_AA_AA;
   case 40:
      goto opADDimm_AA_AA;
   case 41:
      goto opADDimm_AA_AA;
   case 42:
      goto opADDimm_AA_AA;
   case 43:
      goto opADDimm_AA_AA;
   case 44:
      goto opADDimm_AA_AA;
   case 45:
      goto opADDimm_AA_AA;
   case 46:
      goto opADDimm_AA_AA;
   case 47:
      goto opADDimm_AA_AA;
   case 48:
      goto opSUBimmX_AA_AA;
   case 49:
      goto opSUBimm_AA_AA;
   case 50:
      goto opSUBimm_AA_AA;
   case 51:
      goto opSUBimm_AA_AA;
   case 52:
      goto opSUBimm_AA_AA;
   case 53:
      goto opSUBimm_AA_AA;
   case 54:
      goto opSUBimm_AA_AA;
   case 55:
      goto opSUBimm_AA_AA;
   case 56:
      goto opSUBimm_AA_AA;
   case 57:
      goto opSUBimm_AA_AA;
   case 58:
      goto opSUBimm_AA_AA;
   case 59:
      goto opSUBimm_AA_AA;
   case 60:
      goto opSUBimm_AA_AA;
   case 61:
      goto opSUBimm_AA_AA;
   case 62:
      goto opSUBimm_AA_AA;
   case 63:
      goto opSUBimm_AA_AA;
   case 64:
      goto opLDJimm_AA_A;
   case 65:
      goto opLDJimm_AA_A;
   case 66:
      goto opLDJimm_AA_A;
   case 67:
      goto opLDJimm_AA_A;
   case 68:
      goto opLDJimm_AA_A;
   case 69:
      goto opLDJimm_AA_A;
   case 70:
      goto opLDJimm_AA_A;
   case 71:
      goto opLDJimm_AA_A;
   case 72:
      goto opLDJimm_AA_A;
   case 73:
      goto opLDJimm_AA_A;
   case 74:
      goto opLDJimm_AA_A;
   case 75:
      goto opLDJimm_AA_A;
   case 76:
      goto opLDJimm_AA_A;
   case 77:
      goto opLDJimm_AA_A;
   case 78:
      goto opLDJimm_AA_A;
   case 79:
      goto opLDJimm_AA_A;
   case 80:
      goto tJPP_AA_B;		/* redirector */
   case 81:
      goto tJMI_AA_B;		/* redirector */
   case 82:
      goto opJDR_AA_B;
   case 83:
      goto opJLT_AA_B;
   case 84:
      goto opJEQ_AA_B;
   case 85:
      goto opJNC_AA_B;
   case 86:
      goto opJA0_AA_B;
   case 87:
      goto opNOP_AA_B;
   case 88:
      goto opJMP_AA_A;
   case 89:
      goto tJMI_AA_A;		/* redirector */
   case 90:
      goto opJDR_AA_A;
   case 91:
      goto opJLT_AA_A;
   case 92:
      goto opJEQ_AA_A;
   case 93:
      goto opJNC_AA_A;
   case 94:
      goto opJA0_AA_A;
   case 95:
      goto opNOP_AA_A;
   case 96:
      goto opADDdir_AA_AA;
   case 97:
      goto opADDdir_AA_AA;
   case 98:
      goto opADDdir_AA_AA;
   case 99:
      goto opADDdir_AA_AA;
   case 100:
      goto opADDdir_AA_AA;
   case 101:
      goto opADDdir_AA_AA;
   case 102:
      goto opADDdir_AA_AA;
   case 103:
      goto opADDdir_AA_AA;
   case 104:
      goto opADDdir_AA_AA;
   case 105:
      goto opADDdir_AA_AA;
   case 106:
      goto opADDdir_AA_AA;
   case 107:
      goto opADDdir_AA_AA;
   case 108:
      goto opADDdir_AA_AA;
   case 109:
      goto opADDdir_AA_AA;
   case 110:
      goto opADDdir_AA_AA;
   case 111:
      goto opADDdir_AA_AA;
   case 112:
      goto opSUBdir_AA_AA;
   case 113:
      goto opSUBdir_AA_AA;
   case 114:
      goto opSUBdir_AA_AA;
   case 115:
      goto opSUBdir_AA_AA;
   case 116:
      goto opSUBdir_AA_AA;
   case 117:
      goto opSUBdir_AA_AA;
   case 118:
      goto opSUBdir_AA_AA;
   case 119:
      goto opSUBdir_AA_AA;
   case 120:
      goto opSUBdir_AA_AA;
   case 121:
      goto opSUBdir_AA_AA;
   case 122:
      goto opSUBdir_AA_AA;
   case 123:
      goto opSUBdir_AA_AA;
   case 124:
      goto opSUBdir_AA_AA;
   case 125:
      goto opSUBdir_AA_AA;
   case 126:
      goto opSUBdir_AA_AA;
   case 127:
      goto opSUBdir_AA_AA;
   case 128:
      goto opLDPimm_AA_A;
   case 129:
      goto opLDPimm_AA_A;
   case 130:
      goto opLDPimm_AA_A;
   case 131:
      goto opLDPimm_AA_A;
   case 132:
      goto opLDPimm_AA_A;
   case 133:
      goto opLDPimm_AA_A;
   case 134:
      goto opLDPimm_AA_A;
   case 135:
      goto opLDPimm_AA_A;
   case 136:
      goto opLDPimm_AA_A;
   case 137:
      goto opLDPimm_AA_A;
   case 138:
      goto opLDPimm_AA_A;
   case 139:
      goto opLDPimm_AA_A;
   case 140:
      goto opLDPimm_AA_A;
   case 141:
      goto opLDPimm_AA_A;
   case 142:
      goto opLDPimm_AA_A;
   case 143:
      goto opLDPimm_AA_A;
   case 144:
      goto tOUT_AA_A;		/* redirector */
   case 145:
      goto tOUT_AA_A;		/* redirector */
   case 146:
      goto tOUT_AA_A;		/* redirector */
   case 147:
      goto tOUT_AA_A;		/* redirector */
   case 148:
      goto tOUT_AA_A;		/* redirector */
   case 149:
      goto tOUT_AA_A;		/* redirector */
   case 150:
      goto tOUT_AA_A;		/* redirector */
   case 151:
      goto tOUT_AA_A;		/* redirector */
   case 152:
      goto tOUT_AA_A;		/* redirector */
   case 153:
      goto tOUT_AA_A;		/* redirector */
   case 154:
      goto tOUT_AA_A;		/* redirector */
   case 155:
      goto tOUT_AA_A;		/* redirector */
   case 156:
      goto tOUT_AA_A;		/* redirector */
   case 157:
      goto tOUT_AA_A;		/* redirector */
   case 158:
      goto tOUT_AA_A;		/* redirector */
   case 159:
      goto tOUT_AA_A;		/* redirector */
   case 160:
      goto opLDAdir_AA_AA;
   case 161:
      goto opLDAdir_AA_AA;
   case 162:
      goto opLDAdir_AA_AA;
   case 163:
      goto opLDAdir_AA_AA;
   case 164:
      goto opLDAdir_AA_AA;
   case 165:
      goto opLDAdir_AA_AA;
   case 166:
      goto opLDAdir_AA_AA;
   case 167:
      goto opLDAdir_AA_AA;
   case 168:
      goto opLDAdir_AA_AA;
   case 169:
      goto opLDAdir_AA_AA;
   case 170:
      goto opLDAdir_AA_AA;
   case 171:
      goto opLDAdir_AA_AA;
   case 172:
      goto opLDAdir_AA_AA;
   case 173:
      goto opLDAdir_AA_AA;
   case 174:
      goto opLDAdir_AA_AA;
   case 175:
      goto opLDAdir_AA_AA;
   case 176:
      goto opCMPdir_AA_AA;
   case 177:
      goto opCMPdir_AA_AA;
   case 178:
      goto opCMPdir_AA_AA;
   case 179:
      goto opCMPdir_AA_AA;
   case 180:
      goto opCMPdir_AA_AA;
   case 181:
      goto opCMPdir_AA_AA;
   case 182:
      goto opCMPdir_AA_AA;
   case 183:
      goto opCMPdir_AA_AA;
   case 184:
      goto opCMPdir_AA_AA;
   case 185:
      goto opCMPdir_AA_AA;
   case 186:
      goto opCMPdir_AA_AA;
   case 187:
      goto opCMPdir_AA_AA;
   case 188:
      goto opCMPdir_AA_AA;
   case 189:
      goto opCMPdir_AA_AA;
   case 190:
      goto opCMPdir_AA_AA;
   case 191:
      goto opCMPdir_AA_AA;
   case 192:
      goto opLDIdir_AA_A;
   case 193:
      goto opLDIdir_AA_A;
   case 194:
      goto opLDIdir_AA_A;
   case 195:
      goto opLDIdir_AA_A;
   case 196:
      goto opLDIdir_AA_A;
   case 197:
      goto opLDIdir_AA_A;
   case 198:
      goto opLDIdir_AA_A;
   case 199:
      goto opLDIdir_AA_A;
   case 200:
      goto opLDIdir_AA_A;
   case 201:
      goto opLDIdir_AA_A;
   case 202:
      goto opLDIdir_AA_A;
   case 203:
      goto opLDIdir_AA_A;
   case 204:
      goto opLDIdir_AA_A;
   case 205:
      goto opLDIdir_AA_A;
   case 206:
      goto opLDIdir_AA_A;
   case 207:
      goto opLDIdir_AA_A;
   case 208:
      goto opSTAdir_AA_A;
   case 209:
      goto opSTAdir_AA_A;
   case 210:
      goto opSTAdir_AA_A;
   case 211:
      goto opSTAdir_AA_A;
   case 212:
      goto opSTAdir_AA_A;
   case 213:
      goto opSTAdir_AA_A;
   case 214:
      goto opSTAdir_AA_A;
   case 215:
      goto opSTAdir_AA_A;
   case 216:
      goto opSTAdir_AA_A;
   case 217:
      goto opSTAdir_AA_A;
   case 218:
      goto opSTAdir_AA_A;
   case 219:
      goto opSTAdir_AA_A;
   case 220:
      goto opSTAdir_AA_A;
   case 221:
      goto opSTAdir_AA_A;
   case 222:
      goto opSTAdir_AA_A;
   case 223:
      goto opSTAdir_AA_A;
   case 224:
      goto opVDR_AA_A;
   case 225:
      goto opLDJirg_AA_A;
   case 226:
      goto opXLT_AA_AA;
   case 227:
      goto opMULirg_AA_AA;
   case 228:
      goto opLLT_AA_AA;
   case 229:
      goto opWAI_AA_A;
   case 230:
      goto opSTAirg_AA_A;
   case 231:
      goto opADDirg_AA_AA;
   case 232:
      goto opSUBirg_AA_AA;
   case 233:
      goto opANDirg_AA_AA;
   case 234:
      goto opLDAirg_AA_AA;
   case 235:
      goto opLSRe_AA_AA;
   case 236:
      goto opLSLe_AA_AA;
   case 237:
      goto opASRe_AA_AA;
   case 238:
      goto opASRDe_AA_AA;
   case 239:
      goto opLSLDe_AA_AA;
   case 240:
      goto opVIN_AA_A;
   case 241:
      goto opLDJirg_AA_A;
   case 242:
      goto opXLT_AA_AA;
   case 243:
      goto opMULirg_AA_AA;
   case 244:
      goto opLLT_AA_AA;
   case 245:
      goto opWAI_AA_A;
   case 246:
      goto opSTAirg_AA_A;
   case 247:
      goto opAWDirg_AA_AA;
   case 248:
      goto opSUBirg_AA_AA;
   case 249:
      goto opANDirg_AA_AA;
   case 250:
      goto opLDAirg_AA_AA;
   case 251:
      goto opLSRf_AA_AA;
   case 252:
      goto opLSLf_AA_AA;
   case 253:
      goto opASRf_AA_AA;
   case 254:
      goto opASRDf_AA_AA;
   case 255:
      goto opLSLDf_AA_AA;
   } /* switch on opcode */

/* opcode table B -- use this table if the last opcode was a B-reg flip/flop
 * Translation:
 *   Any ACC related routine uses B-reg, and goes to opCodeTblAA
 *   All other instructions will jump to table opCodeTblBB (including
 *     B flip/flop related instructions)
 *   JMI will use current sign of the A-reg
 */

 opCodeTblB:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_B_AA;
   case 1:
      goto opLDAimm_B_AA;
   case 2:
      goto opLDAimm_B_AA;
   case 3:
      goto opLDAimm_B_AA;
   case 4:
      goto opLDAimm_B_AA;
   case 5:
      goto opLDAimm_B_AA;
   case 6:
      goto opLDAimm_B_AA;
   case 7:
      goto opLDAimm_B_AA;
   case 8:
      goto opLDAimm_B_AA;
   case 9:
      goto opLDAimm_B_AA;
   case 10:
      goto opLDAimm_B_AA;
   case 11:
      goto opLDAimm_B_AA;
   case 12:
      goto opLDAimm_B_AA;
   case 13:
      goto opLDAimm_B_AA;
   case 14:
      goto opLDAimm_B_AA;
   case 15:
      goto opLDAimm_B_AA;
   case 16:
      goto opINP_B_AA;
   case 17:
      goto opINP_B_AA;
   case 18:
      goto opINP_B_AA;
   case 19:
      goto opINP_B_AA;
   case 20:
      goto opINP_B_AA;
   case 21:
      goto opINP_B_AA;
   case 22:
      goto opINP_B_AA;
   case 23:
      goto opINP_B_AA;
   case 24:
      goto opINP_B_AA;
   case 25:
      goto opINP_B_AA;
   case 26:
      goto opINP_B_AA;
   case 27:
      goto opINP_B_AA;
   case 28:
      goto opINP_B_AA;
   case 29:
      goto opINP_B_AA;
   case 30:
      goto opINP_B_AA;
   case 31:
      goto opINP_B_AA;
   case 32:
      goto opADDimmX_B_AA;
   case 33:
      goto opADDimm_B_AA;
   case 34:
      goto opADDimm_B_AA;
   case 35:
      goto opADDimm_B_AA;
   case 36:
      goto opADDimm_B_AA;
   case 37:
      goto opADDimm_B_AA;
   case 38:
      goto opADDimm_B_AA;
   case 39:
      goto opADDimm_B_AA;
   case 40:
      goto opADDimm_B_AA;
   case 41:
      goto opADDimm_B_AA;
   case 42:
      goto opADDimm_B_AA;
   case 43:
      goto opADDimm_B_AA;
   case 44:
      goto opADDimm_B_AA;
   case 45:
      goto opADDimm_B_AA;
   case 46:
      goto opADDimm_B_AA;
   case 47:
      goto opADDimm_B_AA;
   case 48:
      goto opSUBimmX_B_AA;
   case 49:
      goto opSUBimm_B_AA;
   case 50:
      goto opSUBimm_B_AA;
   case 51:
      goto opSUBimm_B_AA;
   case 52:
      goto opSUBimm_B_AA;
   case 53:
      goto opSUBimm_B_AA;
   case 54:
      goto opSUBimm_B_AA;
   case 55:
      goto opSUBimm_B_AA;
   case 56:
      goto opSUBimm_B_AA;
   case 57:
      goto opSUBimm_B_AA;
   case 58:
      goto opSUBimm_B_AA;
   case 59:
      goto opSUBimm_B_AA;
   case 60:
      goto opSUBimm_B_AA;
   case 61:
      goto opSUBimm_B_AA;
   case 62:
      goto opSUBimm_B_AA;
   case 63:
      goto opSUBimm_B_AA;
   case 64:
      goto opLDJimm_B_BB;
   case 65:
      goto opLDJimm_B_BB;
   case 66:
      goto opLDJimm_B_BB;
   case 67:
      goto opLDJimm_B_BB;
   case 68:
      goto opLDJimm_B_BB;
   case 69:
      goto opLDJimm_B_BB;
   case 70:
      goto opLDJimm_B_BB;
   case 71:
      goto opLDJimm_B_BB;
   case 72:
      goto opLDJimm_B_BB;
   case 73:
      goto opLDJimm_B_BB;
   case 74:
      goto opLDJimm_B_BB;
   case 75:
      goto opLDJimm_B_BB;
   case 76:
      goto opLDJimm_B_BB;
   case 77:
      goto opLDJimm_B_BB;
   case 78:
      goto opLDJimm_B_BB;
   case 79:
      goto opLDJimm_B_BB;
   case 80:
      goto tJPP_B_BB;		/* redirector */
   case 81:
      goto tJMI_B_BB1;		/* redirector */
   case 82:
      goto opJDR_B_BB;
   case 83:
      goto opJLT_B_BB;
   case 84:
      goto opJEQ_B_BB;
   case 85:
      goto opJNC_B_BB;
   case 86:
      goto opJA0_B_BB;
   case 87:
      goto opNOP_B_BB;
   case 88:
      goto opJMP_B_BB;
   case 89:
      goto tJMI_B_BB2;		/* redirector */
   case 90:
      goto opJDR_B_BB;
   case 91:
      goto opJLT_B_BB;
   case 92:
      goto opJEQ_B_BB;
   case 93:
      goto opJNC_B_BB;
   case 94:
      goto opJA0_B_BB;
   case 95:
      goto opNOP_B_BB;
   case 96:
      goto opADDdir_B_AA;
   case 97:
      goto opADDdir_B_AA;
   case 98:
      goto opADDdir_B_AA;
   case 99:
      goto opADDdir_B_AA;
   case 100:
      goto opADDdir_B_AA;
   case 101:
      goto opADDdir_B_AA;
   case 102:
      goto opADDdir_B_AA;
   case 103:
      goto opADDdir_B_AA;
   case 104:
      goto opADDdir_B_AA;
   case 105:
      goto opADDdir_B_AA;
   case 106:
      goto opADDdir_B_AA;
   case 107:
      goto opADDdir_B_AA;
   case 108:
      goto opADDdir_B_AA;
   case 109:
      goto opADDdir_B_AA;
   case 110:
      goto opADDdir_B_AA;
   case 111:
      goto opADDdir_B_AA;
   case 112:
      goto opSUBdir_B_AA;
   case 113:
      goto opSUBdir_B_AA;
   case 114:
      goto opSUBdir_B_AA;
   case 115:
      goto opSUBdir_B_AA;
   case 116:
      goto opSUBdir_B_AA;
   case 117:
      goto opSUBdir_B_AA;
   case 118:
      goto opSUBdir_B_AA;
   case 119:
      goto opSUBdir_B_AA;
   case 120:
      goto opSUBdir_B_AA;
   case 121:
      goto opSUBdir_B_AA;
   case 122:
      goto opSUBdir_B_AA;
   case 123:
      goto opSUBdir_B_AA;
   case 124:
      goto opSUBdir_B_AA;
   case 125:
      goto opSUBdir_B_AA;
   case 126:
      goto opSUBdir_B_AA;
   case 127:
      goto opSUBdir_B_AA;
   case 128:
      goto opLDPimm_B_BB;
   case 129:
      goto opLDPimm_B_BB;
   case 130:
      goto opLDPimm_B_BB;
   case 131:
      goto opLDPimm_B_BB;
   case 132:
      goto opLDPimm_B_BB;
   case 133:
      goto opLDPimm_B_BB;
   case 134:
      goto opLDPimm_B_BB;
   case 135:
      goto opLDPimm_B_BB;
   case 136:
      goto opLDPimm_B_BB;
   case 137:
      goto opLDPimm_B_BB;
   case 138:
      goto opLDPimm_B_BB;
   case 139:
      goto opLDPimm_B_BB;
   case 140:
      goto opLDPimm_B_BB;
   case 141:
      goto opLDPimm_B_BB;
   case 142:
      goto opLDPimm_B_BB;
   case 143:
      goto opLDPimm_B_BB;
   case 144:
      goto tOUT_B_BB;		/* redirector */
   case 145:
      goto tOUT_B_BB;		/* redirector */
   case 146:
      goto tOUT_B_BB;		/* redirector */
   case 147:
      goto tOUT_B_BB;		/* redirector */
   case 148:
      goto tOUT_B_BB;		/* redirector */
   case 149:
      goto tOUT_B_BB;		/* redirector */
   case 150:
      goto tOUT_B_BB;		/* redirector */
   case 151:
      goto tOUT_B_BB;		/* redirector */
   case 152:
      goto tOUT_B_BB;		/* redirector */
   case 153:
      goto tOUT_B_BB;		/* redirector */
   case 154:
      goto tOUT_B_BB;		/* redirector */
   case 155:
      goto tOUT_B_BB;		/* redirector */
   case 156:
      goto tOUT_B_BB;		/* redirector */
   case 157:
      goto tOUT_B_BB;		/* redirector */
   case 158:
      goto tOUT_B_BB;		/* redirector */
   case 159:
      goto tOUT_B_BB;		/* redirector */
   case 160:
      goto opLDAdir_B_AA;
   case 161:
      goto opLDAdir_B_AA;
   case 162:
      goto opLDAdir_B_AA;
   case 163:
      goto opLDAdir_B_AA;
   case 164:
      goto opLDAdir_B_AA;
   case 165:
      goto opLDAdir_B_AA;
   case 166:
      goto opLDAdir_B_AA;
   case 167:
      goto opLDAdir_B_AA;
   case 168:
      goto opLDAdir_B_AA;
   case 169:
      goto opLDAdir_B_AA;
   case 170:
      goto opLDAdir_B_AA;
   case 171:
      goto opLDAdir_B_AA;
   case 172:
      goto opLDAdir_B_AA;
   case 173:
      goto opLDAdir_B_AA;
   case 174:
      goto opLDAdir_B_AA;
   case 175:
      goto opLDAdir_B_AA;
   case 176:
      goto opCMPdir_B_AA;
   case 177:
      goto opCMPdir_B_AA;
   case 178:
      goto opCMPdir_B_AA;
   case 179:
      goto opCMPdir_B_AA;
   case 180:
      goto opCMPdir_B_AA;
   case 181:
      goto opCMPdir_B_AA;
   case 182:
      goto opCMPdir_B_AA;
   case 183:
      goto opCMPdir_B_AA;
   case 184:
      goto opCMPdir_B_AA;
   case 185:
      goto opCMPdir_B_AA;
   case 186:
      goto opCMPdir_B_AA;
   case 187:
      goto opCMPdir_B_AA;
   case 188:
      goto opCMPdir_B_AA;
   case 189:
      goto opCMPdir_B_AA;
   case 190:
      goto opCMPdir_B_AA;
   case 191:
      goto opCMPdir_B_AA;
   case 192:
      goto opLDIdir_B_BB;
   case 193:
      goto opLDIdir_B_BB;
   case 194:
      goto opLDIdir_B_BB;
   case 195:
      goto opLDIdir_B_BB;
   case 196:
      goto opLDIdir_B_BB;
   case 197:
      goto opLDIdir_B_BB;
   case 198:
      goto opLDIdir_B_BB;
   case 199:
      goto opLDIdir_B_BB;
   case 200:
      goto opLDIdir_B_BB;
   case 201:
      goto opLDIdir_B_BB;
   case 202:
      goto opLDIdir_B_BB;
   case 203:
      goto opLDIdir_B_BB;
   case 204:
      goto opLDIdir_B_BB;
   case 205:
      goto opLDIdir_B_BB;
   case 206:
      goto opLDIdir_B_BB;
   case 207:
      goto opLDIdir_B_BB;
   case 208:
      goto opSTAdir_B_BB;
   case 209:
      goto opSTAdir_B_BB;
   case 210:
      goto opSTAdir_B_BB;
   case 211:
      goto opSTAdir_B_BB;
   case 212:
      goto opSTAdir_B_BB;
   case 213:
      goto opSTAdir_B_BB;
   case 214:
      goto opSTAdir_B_BB;
   case 215:
      goto opSTAdir_B_BB;
   case 216:
      goto opSTAdir_B_BB;
   case 217:
      goto opSTAdir_B_BB;
   case 218:
      goto opSTAdir_B_BB;
   case 219:
      goto opSTAdir_B_BB;
   case 220:
      goto opSTAdir_B_BB;
   case 221:
      goto opSTAdir_B_BB;
   case 222:
      goto opSTAdir_B_BB;
   case 223:
      goto opSTAdir_B_BB;
   case 224:
      goto opVDR_B_BB;
   case 225:
      goto opLDJirg_B_BB;
   case 226:
      goto opXLT_B_AA;
   case 227:
      goto opMULirg_B_AA;
   case 228:
      goto opLLT_B_AA;
   case 229:
      goto opWAI_B_BB;
   case 230:
      goto opSTAirg_B_BB;
   case 231:
      goto opADDirg_B_AA;
   case 232:
      goto opSUBirg_B_AA;
   case 233:
      goto opANDirg_B_AA;
   case 234:
      goto opLDAirg_B_AA;
   case 235:
      goto opLSRe_B_AA;
   case 236:
      goto opLSLe_B_AA;
   case 237:
      goto opASRe_B_AA;
   case 238:
      goto opASRDe_B_AA;
   case 239:
      goto opLSLDe_B_AA;
   case 240:
      goto opVIN_B_BB;
   case 241:
      goto opLDJirg_B_BB;
   case 242:
      goto opXLT_B_AA;
   case 243:
      goto opMULirg_B_AA;
   case 244:
      goto opLLT_B_AA;
   case 245:
      goto opWAI_B_BB;
   case 246:
      goto opSTAirg_B_BB;
   case 247:
      goto opAWDirg_B_AA;
   case 248:
      goto opSUBirg_B_AA;
   case 249:
      goto opANDirg_B_AA;
   case 250:
      goto opLDAirg_B_AA;
   case 251:
      goto opLSRf_B_AA;
   case 252:
      goto opLSLf_B_AA;
   case 253:
      goto opASRf_B_AA;
   case 254:
      goto opASRDf_B_AA;
   case 255:
      goto opLSLDf_B_AA;
   } /* switch on opcode */

/* opcode table BB -- use this table if the last opcode was not an ACC
 * related opcode, but instruction before that was a B-flip/flop instruction.
 * Translation:
 *   Any ACC related routine will use A-reg and go to opCodeTblAA
 *   Any B flip/flop instructions will jump to opCodeTblB
 *   All other instructions will jump to table opCodeTblA
 *   JMI will use the current RCstate of the B-reg
 */

 opCodeTblBB:

   switch (rom[RCregister_PC]) {
   case 0:
      goto opLDAimm_BB_AA;
   case 1:
      goto opLDAimm_BB_AA;
   case 2:
      goto opLDAimm_BB_AA;
   case 3:
      goto opLDAimm_BB_AA;
   case 4:
      goto opLDAimm_BB_AA;
   case 5:
      goto opLDAimm_BB_AA;
   case 6:
      goto opLDAimm_BB_AA;
   case 7:
      goto opLDAimm_BB_AA;
   case 8:
      goto opLDAimm_BB_AA;
   case 9:
      goto opLDAimm_BB_AA;
   case 10:
      goto opLDAimm_BB_AA;
   case 11:
      goto opLDAimm_BB_AA;
   case 12:
      goto opLDAimm_BB_AA;
   case 13:
      goto opLDAimm_BB_AA;
   case 14:
      goto opLDAimm_BB_AA;
   case 15:
      goto opLDAimm_BB_AA;
   case 16:
      goto opINP_BB_AA;
   case 17:
      goto opINP_BB_AA;
   case 18:
      goto opINP_BB_AA;
   case 19:
      goto opINP_BB_AA;
   case 20:
      goto opINP_BB_AA;
   case 21:
      goto opINP_BB_AA;
   case 22:
      goto opINP_BB_AA;
   case 23:
      goto opINP_BB_AA;
   case 24:
      goto opINP_BB_AA;
   case 25:
      goto opINP_BB_AA;
   case 26:
      goto opINP_BB_AA;
   case 27:
      goto opINP_BB_AA;
   case 28:
      goto opINP_BB_AA;
   case 29:
      goto opINP_BB_AA;
   case 30:
      goto opINP_BB_AA;
   case 31:
      goto opINP_BB_AA;
   case 32:
      goto opADDimmX_BB_AA;
   case 33:
      goto opADDimm_BB_AA;
   case 34:
      goto opADDimm_BB_AA;
   case 35:
      goto opADDimm_BB_AA;
   case 36:
      goto opADDimm_BB_AA;
   case 37:
      goto opADDimm_BB_AA;
   case 38:
      goto opADDimm_BB_AA;
   case 39:
      goto opADDimm_BB_AA;
   case 40:
      goto opADDimm_BB_AA;
   case 41:
      goto opADDimm_BB_AA;
   case 42:
      goto opADDimm_BB_AA;
   case 43:
      goto opADDimm_BB_AA;
   case 44:
      goto opADDimm_BB_AA;
   case 45:
      goto opADDimm_BB_AA;
   case 46:
      goto opADDimm_BB_AA;
   case 47:
      goto opADDimm_BB_AA;
   case 48:
      goto opSUBimmX_BB_AA;
   case 49:
      goto opSUBimm_BB_AA;
   case 50:
      goto opSUBimm_BB_AA;
   case 51:
      goto opSUBimm_BB_AA;
   case 52:
      goto opSUBimm_BB_AA;
   case 53:
      goto opSUBimm_BB_AA;
   case 54:
      goto opSUBimm_BB_AA;
   case 55:
      goto opSUBimm_BB_AA;
   case 56:
      goto opSUBimm_BB_AA;
   case 57:
      goto opSUBimm_BB_AA;
   case 58:
      goto opSUBimm_BB_AA;
   case 59:
      goto opSUBimm_BB_AA;
   case 60:
      goto opSUBimm_BB_AA;
   case 61:
      goto opSUBimm_BB_AA;
   case 62:
      goto opSUBimm_BB_AA;
   case 63:
      goto opSUBimm_BB_AA;
   case 64:
      goto opLDJimm_BB_A;
   case 65:
      goto opLDJimm_BB_A;
   case 66:
      goto opLDJimm_BB_A;
   case 67:
      goto opLDJimm_BB_A;
   case 68:
      goto opLDJimm_BB_A;
   case 69:
      goto opLDJimm_BB_A;
   case 70:
      goto opLDJimm_BB_A;
   case 71:
      goto opLDJimm_BB_A;
   case 72:
      goto opLDJimm_BB_A;
   case 73:
      goto opLDJimm_BB_A;
   case 74:
      goto opLDJimm_BB_A;
   case 75:
      goto opLDJimm_BB_A;
   case 76:
      goto opLDJimm_BB_A;
   case 77:
      goto opLDJimm_BB_A;
   case 78:
      goto opLDJimm_BB_A;
   case 79:
      goto opLDJimm_BB_A;
   case 80:
      goto tJPP_BB_B;		/* redirector */
   case 81:
      goto tJMI_BB_B;		/* redirector */
   case 82:
      goto opJDR_BB_B;
   case 83:
      goto opJLT_BB_B;
   case 84:
      goto opJEQ_BB_B;
   case 85:
      goto opJNC_BB_B;
   case 86:
      goto opJA0_BB_B;
   case 87:
      goto opNOP_BB_B;
   case 88:
      goto opJMP_BB_A;
   case 89:
      goto tJMI_BB_A;		/* redirector */
   case 90:
      goto opJDR_BB_A;
   case 91:
      goto opJLT_BB_A;
   case 92:
      goto opJEQ_BB_A;
   case 93:
      goto opJNC_BB_A;
   case 94:
      goto opJA0_BB_A;
   case 95:
      goto opNOP_BB_A;
   case 96:
      goto opADDdir_BB_AA;
   case 97:
      goto opADDdir_BB_AA;
   case 98:
      goto opADDdir_BB_AA;
   case 99:
      goto opADDdir_BB_AA;
   case 100:
      goto opADDdir_BB_AA;
   case 101:
      goto opADDdir_BB_AA;
   case 102:
      goto opADDdir_BB_AA;
   case 103:
      goto opADDdir_BB_AA;
   case 104:
      goto opADDdir_BB_AA;
   case 105:
      goto opADDdir_BB_AA;
   case 106:
      goto opADDdir_BB_AA;
   case 107:
      goto opADDdir_BB_AA;
   case 108:
      goto opADDdir_BB_AA;
   case 109:
      goto opADDdir_BB_AA;
   case 110:
      goto opADDdir_BB_AA;
   case 111:
      goto opADDdir_BB_AA;
   case 112:
      goto opSUBdir_BB_AA;
   case 113:
      goto opSUBdir_BB_AA;
   case 114:
      goto opSUBdir_BB_AA;
   case 115:
      goto opSUBdir_BB_AA;
   case 116:
      goto opSUBdir_BB_AA;
   case 117:
      goto opSUBdir_BB_AA;
   case 118:
      goto opSUBdir_BB_AA;
   case 119:
      goto opSUBdir_BB_AA;
   case 120:
      goto opSUBdir_BB_AA;
   case 121:
      goto opSUBdir_BB_AA;
   case 122:
      goto opSUBdir_BB_AA;
   case 123:
      goto opSUBdir_BB_AA;
   case 124:
      goto opSUBdir_BB_AA;
   case 125:
      goto opSUBdir_BB_AA;
   case 126:
      goto opSUBdir_BB_AA;
   case 127:
      goto opSUBdir_BB_AA;
   case 128:
      goto opLDPimm_BB_A;
   case 129:
      goto opLDPimm_BB_A;
   case 130:
      goto opLDPimm_BB_A;
   case 131:
      goto opLDPimm_BB_A;
   case 132:
      goto opLDPimm_BB_A;
   case 133:
      goto opLDPimm_BB_A;
   case 134:
      goto opLDPimm_BB_A;
   case 135:
      goto opLDPimm_BB_A;
   case 136:
      goto opLDPimm_BB_A;
   case 137:
      goto opLDPimm_BB_A;
   case 138:
      goto opLDPimm_BB_A;
   case 139:
      goto opLDPimm_BB_A;
   case 140:
      goto opLDPimm_BB_A;
   case 141:
      goto opLDPimm_BB_A;
   case 142:
      goto opLDPimm_BB_A;
   case 143:
      goto opLDPimm_BB_A;
   case 144:
      goto tOUT_BB_A;		/* redirector */
   case 145:
      goto tOUT_BB_A;		/* redirector */
   case 146:
      goto tOUT_BB_A;		/* redirector */
   case 147:
      goto tOUT_BB_A;		/* redirector */
   case 148:
      goto tOUT_BB_A;		/* redirector */
   case 149:
      goto tOUT_BB_A;		/* redirector */
   case 150:
      goto tOUT_BB_A;		/* redirector */
   case 151:
      goto tOUT_BB_A;		/* redirector */
   case 152:
      goto tOUT_BB_A;		/* redirector */
   case 153:
      goto tOUT_BB_A;		/* redirector */
   case 154:
      goto tOUT_BB_A;		/* redirector */
   case 155:
      goto tOUT_BB_A;		/* redirector */
   case 156:
      goto tOUT_BB_A;		/* redirector */
   case 157:
      goto tOUT_BB_A;		/* redirector */
   case 158:
      goto tOUT_BB_A;		/* redirector */
   case 159:
      goto tOUT_BB_A;		/* redirector */
   case 160:
      goto opLDAdir_BB_AA;
   case 161:
      goto opLDAdir_BB_AA;
   case 162:
      goto opLDAdir_BB_AA;
   case 163:
      goto opLDAdir_BB_AA;
   case 164:
      goto opLDAdir_BB_AA;
   case 165:
      goto opLDAdir_BB_AA;
   case 166:
      goto opLDAdir_BB_AA;
   case 167:
      goto opLDAdir_BB_AA;
   case 168:
      goto opLDAdir_BB_AA;
   case 169:
      goto opLDAdir_BB_AA;
   case 170:
      goto opLDAdir_BB_AA;
   case 171:
      goto opLDAdir_BB_AA;
   case 172:
      goto opLDAdir_BB_AA;
   case 173:
      goto opLDAdir_BB_AA;
   case 174:
      goto opLDAdir_BB_AA;
   case 175:
      goto opLDAdir_BB_AA;
   case 176:
      goto opCMPdir_BB_AA;
   case 177:
      goto opCMPdir_BB_AA;
   case 178:
      goto opCMPdir_BB_AA;
   case 179:
      goto opCMPdir_BB_AA;
   case 180:
      goto opCMPdir_BB_AA;
   case 181:
      goto opCMPdir_BB_AA;
   case 182:
      goto opCMPdir_BB_AA;
   case 183:
      goto opCMPdir_BB_AA;
   case 184:
      goto opCMPdir_BB_AA;
   case 185:
      goto opCMPdir_BB_AA;
   case 186:
      goto opCMPdir_BB_AA;
   case 187:
      goto opCMPdir_BB_AA;
   case 188:
      goto opCMPdir_BB_AA;
   case 189:
      goto opCMPdir_BB_AA;
   case 190:
      goto opCMPdir_BB_AA;
   case 191:
      goto opCMPdir_BB_AA;
   case 192:
      goto opLDIdir_BB_A;
   case 193:
      goto opLDIdir_BB_A;
   case 194:
      goto opLDIdir_BB_A;
   case 195:
      goto opLDIdir_BB_A;
   case 196:
      goto opLDIdir_BB_A;
   case 197:
      goto opLDIdir_BB_A;
   case 198:
      goto opLDIdir_BB_A;
   case 199:
      goto opLDIdir_BB_A;
   case 200:
      goto opLDIdir_BB_A;
   case 201:
      goto opLDIdir_BB_A;
   case 202:
      goto opLDIdir_BB_A;
   case 203:
      goto opLDIdir_BB_A;
   case 204:
      goto opLDIdir_BB_A;
   case 205:
      goto opLDIdir_BB_A;
   case 206:
      goto opLDIdir_BB_A;
   case 207:
      goto opLDIdir_BB_A;
   case 208:
      goto opSTAdir_BB_A;
   case 209:
      goto opSTAdir_BB_A;
   case 210:
      goto opSTAdir_BB_A;
   case 211:
      goto opSTAdir_BB_A;
   case 212:
      goto opSTAdir_BB_A;
   case 213:
      goto opSTAdir_BB_A;
   case 214:
      goto opSTAdir_BB_A;
   case 215:
      goto opSTAdir_BB_A;
   case 216:
      goto opSTAdir_BB_A;
   case 217:
      goto opSTAdir_BB_A;
   case 218:
      goto opSTAdir_BB_A;
   case 219:
      goto opSTAdir_BB_A;
   case 220:
      goto opSTAdir_BB_A;
   case 221:
      goto opSTAdir_BB_A;
   case 222:
      goto opSTAdir_BB_A;
   case 223:
      goto opSTAdir_BB_A;
   case 224:
      goto opVDR_BB_A;
   case 225:
      goto opLDJirg_BB_A;
   case 226:
      goto opXLT_BB_AA;
   case 227:
      goto opMULirg_BB_AA;
   case 228:
      goto opLLT_BB_AA;
   case 229:
      goto opWAI_BB_A;
   case 230:
      goto opSTAirg_BB_A;
   case 231:
      goto opADDirg_BB_AA;
   case 232:
      goto opSUBirg_BB_AA;
   case 233:
      goto opANDirg_BB_AA;
   case 234:
      goto opLDAirg_BB_AA;
   case 235:
      goto opLSRe_BB_AA;
   case 236:
      goto opLSLe_BB_AA;
   case 237:
      goto opASRe_BB_AA;
   case 238:
      goto opASRDe_BB_AA;
   case 239:
      goto opLSLDe_BB_AA;
   case 240:
      goto opVIN_BB_A;
   case 241:
      goto opLDJirg_BB_A;
   case 242:
      goto opXLT_BB_AA;
   case 243:
      goto opMULirg_BB_AA;
   case 244:
      goto opLLT_BB_AA;
   case 245:
      goto opWAI_BB_A;
   case 246:
      goto opSTAirg_BB_A;
   case 247:
      goto opAWDirg_BB_AA;
   case 248:
      goto opSUBirg_BB_AA;
   case 249:
      goto opANDirg_BB_AA;
   case 250:
      goto opLDAirg_BB_AA;
   case 251:
      goto opLSRf_BB_AA;
   case 252:
      goto opLSLf_BB_AA;
   case 253:
      goto opASRf_BB_AA;
   case 254:
      goto opASRDf_BB_AA;
   case 255:
      goto opLSLDf_BB_AA;
   } /* switch on opcode */

   /* the actual opcode code; each piece should be careful to (1) set the
      correct RCstate (2) increment the program counter as necessary (3) piss
      with the flags as needed otherwise the next opcode will be completely
      buggered. */

 opINP_A_AA:
 opINP_AA_AA:
 opINP_BB_AA:
   /* bottom 4 bits of opcode are the position of the bit we want; obtain
      input value, shift over that no, and truncate to last bit. NOTE:
      Masking 0x07 does interesting things on Sundance and others, but
      masking 0x0F makes RipOff and others actually work :) */
#ifdef DEBUGGING_STARHAWK
   RCcmp_new = (ioInputs >> (rom[RCregister_PC] & 0x0FF)) & 0x01; // some of the ioInputs care > 0x10000 - don't know how to extact data.
#else
   RCcmp_new = (ioInputs >> (rom[RCregister_PC] & 0x0F)) & 0x01;
#endif

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;	/* save old accB */
   RCregister_A = RCcmp_new;	/* load new accB; zero other bits */

   RCregister_PC++;
   jumpCineRet_AA;




 opINP_B_AA:
   /* bottom 3 bits of opcode are the position of the bit we want; obtain
      Switches value, shift over that no, and truncate to last bit. */
   RCcmp_new = (ioSwitches >> (rom[RCregister_PC] & 0x07)) & 0x01;

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* save old accB */
   RCregister_B = RCcmp_new;	/* load new accB; zero other bits */

   RCregister_PC++;
   jumpCineRet_AA;

 opOUTbi_A_A:
 opOUTbi_AA_A:
 opOUTbi_BB_A:

   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   RCvgColour = ((RCregister_A & 0x01) << 3) | 0x07;

   jumpCineRet_A;

 opOUT16_A_A:
 opOUT16_AA_A:
 opOUT16_BB_A:

   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   if ((RCregister_A & 0xFF) != 1) {
      RCvgColour = RCFromX & 0x0F;

      if (!RCvgColour) {
	 RCvgColour = 1;
      }

   }

   jumpCineRet_A;

 opOUTsnd_A:
   temp_byte = 0x01 << (rom[RCregister_PC] & 0x07);

   if (!(RCregister_A & 0x01)) {
      goto opOUT_Aset;
   }

   temp_byte = (!temp_byte);	/* BUG? Should this not be ~temp_byte */
   ioOutputs &= temp_byte;

   if ((rom[RCregister_PC] & 0x07) == 0x05) {
      goto opOUT_Aq;
   }

   jumpCineRet_A;

 opOUT_Aq:
   /* reset coin counter */
   jumpCineRet_A;

 opOUT_Aset:
   ioOutputs |= temp_byte;
   jumpCineRet_A;

 opOUT64_A_A:
 opOUT64_AA_A:
 opOUT64_BB_A:
   jumpCineRet_A;

 opOUTWW_A_A:
 opOUTWW_AA_A:
 opOUTWW_BB_A:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_A;
   }

   if ((RCregister_A & 0xFF) == 1) {
      temp_word = (!RCFromX) & 0xFFF;
      if (!temp_word) {		/* black */
	 RCvgColour = 0;
      } else {			/* non-black */
	 if (temp_word & 0x0888) {
	    /* bright */
	    temp_word_2 = ((temp_word << 4) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    RCvgColour = (temp_byte & 0x07) + 7;
	 } else if (temp_word & 0x0444) {
	    /* dim bits */
	    temp_word_2 = ((temp_word << 5) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    temp_word_2 = ((temp_word << 3) & 0x8000) >> 15;
	    temp_byte = (temp_byte << 1) + temp_word_2;

	    RCvgColour = (temp_byte & 0x07);
	 } else {
	    /* dim white */
	    RCvgColour = 0x0F;
	 }
      }
   }
   /* colour change? == 1 */
   jumpCineRet_A;

 opOUTbi_B_BB:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_B;
   }

   RCvgColour = ((RCregister_B & 0x01) << 3) | 0x07;

   jumpCineRet_BB;

 opOUT16_B_BB:
   temp_byte = rom[RCregister_PC] & 0x07;
   RCregister_PC++;

   if (temp_byte - 0x06) {
      goto opOUTsnd_B;
   }

   if ((RCregister_B & 0xFF) != 1) {
      RCvgColour = RCFromX & 0x0F;

      if (!RCvgColour) {
	 RCvgColour = 1;
      }

   }

   jumpCineRet_BB;

 opOUTsnd_B:
   jumpCineRet_BB;

 opOUT64_B_BB:
   jumpCineRet_BB;

 opOUTWW_B_BB:
   jumpCineRet_BB;

   /* LDA imm (0x) */
 opLDAimm_A_AA:
 opLDAimm_AA_AA:
 opLDAimm_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* pick up immediate value */
   temp_word <<= 8;		/* LDAimm is the HIGH nibble! */

   RCcmp_new = temp_word;	/* set new comparison flag */

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);	/* ??? clear carry? */

   RCcmp_old = RCregister_A;	/* step back cmp flag */
   RCregister_A = temp_word;	/* set the register */

   RCregister_PC++;		/* increment PC */
   jumpCineRet_AA;		/* swap RCstate and end opcode */

 opLDAimm_B_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* pick up immediate value */
   temp_word <<= 8;		/* LDAimm is the HIGH nibble! */

   RCcmp_new = temp_word;	/* set new comparison flag */

   SETA0 (RCregister_A);	/* save old accA bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* step back cmp flag */
   RCregister_B = temp_word;	/* set the register */

   RCregister_PC++;		/* increment PC */
   jumpCineRet_AA;

 opLDAdir_A_AA:
 opLDAdir_AA_AA:
 opLDAdir_BB_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */

   RCcmp_new = RCram[RCregister_I];	/* new acc value */

   SETA0 (RCregister_A);	/* back up bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;	/* store old acc */
   RCregister_A = RCcmp_new;	/* store new acc */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLDAdir_B_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */

   RCcmp_new = RCram[RCregister_I];	/* new acc value */

   SETA0 (RCregister_A);	/* back up bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;	/* store old acc */
   RCregister_B = RCcmp_new;	/* store new acc */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLDAirg_A_AA:
 opLDAirg_AA_AA:
 opLDAirg_BB_AA:

   RCcmp_new = RCram[RCregister_I];

   SETA0 (RCregister_A);
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;
   RCregister_A = RCcmp_new;

   RCregister_PC++;
   jumpCineRet_AA;

 opLDAirg_B_AA:
   RCcmp_new = RCram[RCregister_I];

   SETA0 (RCregister_A);
   SETFC (RCregister_A);

   RCcmp_old = RCregister_B;
   RCregister_B = RCcmp_new;

   RCregister_PC++;
   jumpCineRet_AA;

   /* ADD imm */
 opADDimm_A_AA:
 opADDimm_AA_AA:
 opADDimm_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* get imm value */

   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* store old acc for later */

   RCregister_A += temp_word;	/* add values */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC++;
   jumpCineRet_AA;

 opADDimm_B_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* get imm value */

   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_B;	/* store old acc for later */

   RCregister_B += temp_word;	/* add values */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC++;
   jumpCineRet_AA;

   /* ADD imm extended */
 opADDimmX_A_AA:
 opADDimmX_AA_AA:
 opADDimmX_BB_AA:
   RCcmp_new = rom[RCregister_PC + 1];	/* get extended value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* store old acc for later */

   RCregister_A += RCcmp_new;	/* add values */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opADDimmX_B_AA:
   RCcmp_new = rom[RCregister_PC + 1];	/* get extended value */
   SETA0 (RCregister_A);	/* save old accA bit0 */
   RCcmp_old = RCregister_B;	/* store old acc for later */

   RCregister_B += RCcmp_new;	/* add values */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;	/* toss out >12bit carry */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opADDdir_A_AA:
 opADDdir_AA_AA:
 opADDdir_BB_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];	/* fetch imm real value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_A;	/* store old acc value */

   RCregister_A += RCcmp_new;	/* do acc operation */
   SETFC (RCregister_A);	/* store carry and extra */
   RCregister_A &= 0xFFF;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opADDdir_B_AA:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];	/* fetch imm real value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_B;	/* store old acc value */

   RCregister_B += RCcmp_new;	/* do acc operation */
   SETFC (RCregister_B);	/* store carry and extra */
   RCregister_B &= 0xFFF;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opAWDirg_A_AA:
 opAWDirg_AA_AA:
 opAWDirg_BB_AA:
 opADDirg_A_AA:
 opADDirg_AA_AA:
 opADDirg_BB_AA:

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   RCregister_A += RCcmp_new;
   SETFC (RCregister_A);
   RCregister_A &= 0xFFF;

   RCregister_PC++;
   jumpCineRet_AA;

 opAWDirg_B_AA:
 opADDirg_B_AA:
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B += RCcmp_new;
   SETFC (RCregister_B);
   RCregister_B &= 0xFFF;

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBimm_A_AA:
 opSUBimm_AA_AA:
 opSUBimm_BB_AA:
   /* SUBtractions are negate-and-add instructions of the CCPU; what a pain in the ass. */
   temp_word = rom[RCregister_PC] & 0x0F;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opSUBimm_B_AA:
   /* SUBtractions are negate-and-add instructions of the CCPU; what a pain in the ass. */
   temp_word = rom[RCregister_PC] & 0x0F;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opSUBimmX_A_AA:
 opSUBimmX_AA_AA:
 opSUBimmX_BB_AA:

   temp_word = rom[RCregister_PC + 1];	/* snag imm value */

   RCcmp_new = temp_word;	/* save cmp value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_A;	/* back up regA */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opSUBimmX_B_AA:

   temp_word = rom[RCregister_PC + 1];	/* snag imm value */

   RCcmp_new = temp_word;	/* save cmp value */
   SETA0 (RCregister_A);	/* store bit0 */
   RCcmp_old = RCregister_B;	/* back up regA */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC += 2;		/* bump PC */
   jumpCineRet_AA;

 opSUBdir_A_AA:
 opSUBdir_AA_AA:
 opSUBdir_BB_AA:
   temp_word = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_word;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBdir_B_AA:

   temp_byte = rom[RCregister_PC] & 0x0F;	/* fetch imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set regI addr */

   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBirg_A_AA:
 opSUBirg_AA_AA:
 opSUBirg_BB_AA:
   /* sub [i] */
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_A += temp_word;	/* add */
   SETFC (RCregister_A);	/* pick up top bits */
   RCregister_A &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

 opSUBirg_B_AA:
   /* sub [i] */
   RCcmp_new = RCram[RCregister_I];
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word = (RCcmp_new ^ 0xFFF) + 1;	/* ones compliment */
   RCregister_B += temp_word;	/* add */
   SETFC (RCregister_B);	/* pick up top bits */
   RCregister_B &= 0xFFF;	/* mask final regA value */

   RCregister_PC++;
   jumpCineRet_AA;

   /* CMP dir */
 opCMPdir_A_AA:
 opCMPdir_AA_AA:
 opCMPdir_BB_AA:
   /* compare direct mode; don't modify regs, just set carry flag or not. */

   temp_byte = rom[RCregister_PC] & 0x0F;	/* obtain relative addr */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* build real addr */

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* backup bit0 */
   RCcmp_old = RCregister_A;	/* backup old acc */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   temp_word += RCregister_A;
   SETFC (temp_word);		/* pick up top bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opCMPdir_B_AA:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* obtain relative addr */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* build real addr */

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;	/* new acc value */
   SETA0 (RCregister_A);	/* backup bit0 */
   RCcmp_old = RCregister_B;	/* backup old acc */

   temp_word = (temp_word ^ 0xFFF) + 1;	/* ones compliment */
   temp_word += RCregister_B;
   SETFC (temp_word);		/* pick up top bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

   /* AND [i] */
 opANDirg_A_AA:
 opANDirg_AA_AA:
 opANDirg_BB_AA:
   RCcmp_new = RCram[RCregister_I];	/* new acc value */
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;

   RCregister_A &= RCcmp_new;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opANDirg_B_AA:
   RCcmp_new = RCram[RCregister_I];	/* new acc value */
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B &= RCcmp_new;

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

   /* LDJ imm */
 opLDJimm_A_A:
 opLDJimm_AA_A:
 opLDJimm_BB_A:
   temp_byte = rom[RCregister_PC + 1];	/* upper part of address */
   temp_byte = (temp_byte << 4) | (temp_byte >> 4); /* Silly CCPU; Swap nibbles */

   /* put the upper 8 bits above the existing 4 bits */
   RCregister_J = (rom[RCregister_PC] & 0x0F) | (temp_byte << 4);

   RCregister_PC += 2;
   jumpCineRet_A;

 opLDJimm_B_BB:
   temp_byte = rom[RCregister_PC + 1];	/* upper part of address */
   temp_byte = (temp_byte << 4) | (temp_byte >> 4); /* Silly CCPU; Swap nibbles */

   /* put the upper 8 bits above the existing 4 bits */
   RCregister_J = (rom[RCregister_PC] & 0x0F) | (temp_byte << 4);

   RCregister_PC += 2;
   jumpCineRet_BB;

   /* LDJ irg */
 opLDJirg_A_A:
 opLDJirg_AA_A:
 opLDJirg_BB_A:
   /* load J reg from value at last dir addr */
   RCregister_J = RCram[RCregister_I];
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opLDJirg_B_BB:
   RCregister_J = RCram[RCregister_I];
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

   /* LDP imm */
 opLDPimm_A_A:
 opLDPimm_AA_A:
 opLDPimm_BB_A:
   /* load page register from immediate */
   RCregister_P = rom[RCregister_PC] & 0x0F;	/* set page register */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_A;

 opLDPimm_B_BB:
   /* load page register from immediate */
   RCregister_P = rom[RCregister_PC] & 0x0F;	/* set page register */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_BB;

   /* LDI dir */
 opLDIdir_A_A:
 opLDIdir_AA_A:
 opLDIdir_BB_A:
   /* load regI directly .. */

   temp_byte = (RCregister_P << 4) +	/* get rampage ... */
    (rom[RCregister_PC] & 0x0F);	/* and imm half of ramaddr.. */

   RCregister_I = RCram[temp_byte] & 0xFF;	/* set/mask new RCregister_I */

   RCregister_PC++;
   jumpCineRet_A;

 opLDIdir_B_BB:
   temp_byte = (RCregister_P << 4) + (rom[RCregister_PC] & 0x0F);	/* get rampage ... and imm half of ramaddr.. */
   RCregister_I = RCram[temp_byte] & 0xFF;	/* set/mask new RCregister_I */
   RCregister_PC++;
   jumpCineRet_BB;

   /* STA dir */
 opSTAdir_A_A:
 opSTAdir_AA_A:
 opSTAdir_BB_A:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */
   RCram[RCregister_I] = RCregister_A;	/* store acc to RCram */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_A;

 opSTAdir_B_BB:
   temp_byte = rom[RCregister_PC] & 0x0F;	/* snag imm value */
   RCregister_I = (RCregister_P << 4) + temp_byte;	/* set I register */
   RCram[RCregister_I] = RCregister_B;	/* store acc to RCram */
   RCregister_PC++;		/* inc PC */
   jumpCineRet_BB;

   /* STA irg */
 opSTAirg_A_A:
 opSTAirg_AA_A:
 opSTAirg_BB_A:
   /* STA into address specified in regI. Nice and easy :) */
   RCram[RCregister_I] = RCregister_A;	/* store acc */
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opSTAirg_B_BB:
   RCram[RCregister_I] = RCregister_B;	/* store acc */
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

   /* XLT */
 opXLT_A_AA:
 opXLT_AA_AA:
 opXLT_BB_AA:
   /* XLT is weird; it loads the current accumulator with the bytevalue at
      ROM location pointed to by the accumulator; this allows the programto
      read the programitself..  NOTE! Next opcode is *IGNORED!* because of a
      twisted side-effect */

   RCcmp_new = rom[(RCregister_PC & 0xF000) + RCregister_A];	/* store new acc value */
   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;	/* back up acc */
   RCregister_A = RCcmp_new;	/* new acc value */
   RCregister_PC += 2;		/* bump PC twice because XLT is fucked up */
   jumpCineRet_AA;

 opXLT_B_AA:

   RCcmp_new = rom[(RCregister_PC & 0xF000) + RCregister_B];	/* store new acc value */
   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;	/* back up acc */
   RCregister_B = RCcmp_new;	/* new acc value */
   RCregister_PC += 2;		/* bump PC twice because XLT is fucked up */
   jumpCineRet_AA;

   /* MUL [i] */
 opMULirg_A_AA:
 opMULirg_AA_AA:
 opMULirg_BB_AA:
   /* MUL's usually happen in batches, so a slight speed bump can be gained
      by checking for multiple instances and handling in here, without going
      through the main loop for each. */
   temp_word = RCram[RCregister_I];	/* pick up ramvalue */
   RCcmp_new = temp_word;
   temp_word <<= 4;		/* shift into ADD position */
   RCregister_B <<= 4;		/* get sign bit 15 */
   RCregister_B |= (RCregister_A >> 8);	/* bring in A high nibble */
   RCregister_A = ((RCregister_A & 0xFF) << 8) | (rom[RCregister_PC] & 0xFF);	/* shift over 8 bits and pick up opcode */
   temp_byte = rom[RCregister_PC] & 0xFF;	/* (for ease and speed) */

   /* handle multiple consecutive MUL's */

   RCregister_PC++;		/* inc PC */

   if (rom[RCregister_PC] != temp_byte) {	/* next opcode is a MUL? */
      goto opMUL1;		/* no? skip multiples... */
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL2;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL3;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL4;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL5;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL6;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL7;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL8;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL9;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL10;
   }

   RCregister_PC++;		/* repeat above */
   if (rom[RCregister_PC] != temp_byte) {
      goto opMUL11;
   }

//opMUL12:
   RCregister_PC++;		/* we don't bother to check for more than 12 multiple
				   occurances, so just inc the PC and don't worry about it. */

   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	temp_word_2; /* rotate right ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL11;
   }
   RCregister_B += temp_word;

 opMUL11:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL10;
   }
   RCregister_B += temp_word;

 opMUL10:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL9;
   }
   RCregister_B += temp_word;

 opMUL9:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL8;
   }
   RCregister_B += temp_word;

 opMUL8:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL7;
   }
   RCregister_B += temp_word;

 opMUL7:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL6;
   }
   RCregister_B += temp_word;

 opMUL6:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL5;
   }
   RCregister_B += temp_word;

 opMUL5:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL4;
   }
   RCregister_B += temp_word;

 opMUL4:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL3;
   }
   RCregister_B += temp_word;

 opMUL3:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL2;
   }
   RCregister_B += temp_word;

 opMUL2:
   temp_word_2 = (RCregister_B & 0x01) << 15;	/* get low bit for rotation */
   RCregister_B = SAR16 (RCregister_B, 1);	/* signed arith right 1 */
   RCregister_A = (RCregister_A >> 1) |	/* rotate right ... */
    temp_word_2;		/* ... via carry flag */
   if (!(RCregister_A & 0x80)) {
      goto opMUL1;
   }
   RCregister_B += temp_word;

 opMUL1:

   if (RCregister_A & 0x100) {	/* 1bit shifted out? */
      goto opMULshf;
   }

   RCregister_A = (RCregister_A >> 8) |	/* Bhigh | Alow */
    ((RCregister_B & 0xFF) << 8);

   temp_word = RCregister_A & 0xFFF;

   SETA0 (temp_word & 0xFF);	/* store bit0 */
   RCcmp_old = temp_word;

   temp_word += RCcmp_new;
   SETFC (temp_word);

   RCregister_A >>= 1;
   RCregister_A &= 0xFFF;

   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opMULshf:			/* part of opMULirg */

   RCregister_A = (RCregister_A >> 8) | ((RCregister_B & 0xFF) << 8);

   SETA0 (RCregister_A & 0xFF);	/* store bit0 */

   RCregister_A >>= 1;
   RCregister_A &= 0xFFF;

   RCregister_B = SAR16 (RCregister_B, 4);
   RCcmp_old = RCregister_B & 0x0F;

   RCregister_B = SAR16 (RCregister_B, 1);

   RCregister_B &= 0xFFF;
   RCregister_B += RCcmp_new;

   SETFC (RCregister_B);

   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opMULirg_B_AA:
   RCregister_PC++;

   temp_word = RCram[RCregister_I];
   RCcmp_new = temp_word;
   RCcmp_old = RCregister_B;
   SETA0 (RCregister_A & 0xFF);

   RCregister_B <<= 4;

   RCregister_B = SAR16 (RCregister_B, 5);

   if (RCregister_A & 0x01) {
      goto opMULirgB1;
   }

   temp_word += RCregister_B;
   SETFC (temp_word);

   jumpCineRet_AA;

 opMULirgB1:
   RCregister_B += temp_word;
   SETFC (RCregister_B);
   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

   /* LSRe */
 opLSRe_A_AA:
 opLSRe_AA_AA:
 opLSRe_BB_AA:
   /* EB; right shift pure; fill new bit with zero. */
   temp_word = 0x0BEB;

   RCregister_PC++;

   if (rom[RCregister_PC] == 0xEB) {
      goto opLSRe_A0;		/* multiples */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A >>= 1;
   jumpCineRet_AA;

 opLSRe_A0:
   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEB) {
      goto opLSRe_A1;
   }

   RCregister_A >>= 1;
   RCregister_PC++;

 opLSRe_A1:
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A >>= 1;
   jumpCineRet_AA;

 opLSRe_B_AA:
   // UNFINISHED ("opLSRe B 1\n");
   // added code for opcode missing from boxingbugs
   /* ??; right shift pure; fill new bit with zero. */
   temp_word = 0x0BEB;

   RCregister_PC++;

   RCcmp_new = temp_word;
   SETA0 (RCregister_B);       // OR Register_A ??? check against Mame?
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);

   RCregister_B >>= 1;
   // end of added code
   jumpCineRet_AA;

 opLSRf_A_AA:
 opLSRf_AA_AA:
 opLSRf_BB_AA:
   //UNFINISHED ("opLSRf 1\n");
   // added code for opcode missing from boxingbugs
   RCregister_PC++;
   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);

   RCcmp_old = RCregister_A;
   temp_word += RCregister_A;

   SETFC (temp_word);

   RCregister_A >>= 1;
   RCregister_A &= 0xFFF;
   // end of added code
   jumpCineRet_AA;

 opLSRf_B_AA:
   //UNFINISHED ("opLSRf 2\n");
   // added code for opcode missing from boxingbugs
   RCregister_PC++;
   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);

   RCcmp_old = RCregister_B;
   temp_word += RCregister_B;

   SETFC (temp_word);

   RCregister_B >>= 1;
   RCregister_B &= 0xFFF;
   // end of added code
   jumpCineRet_AA;

 opLSLe_A_AA:
 opLSLe_AA_AA:
 opLSLe_BB_AA:
   /* EC; left shift pure; fill new bit with zero */
   /* This version supports multiple consecutive LSLe's; the older version
      only did one at a time. I'm changing it to make tracing easier (as its
      comperable to Zonn's) */

   RCregister_PC++;
   temp_word = 0x0CEC;

   if (rom[RCregister_PC] == 0xEC) {
      goto opLSLe_A0;		/* do multiples */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opLSLe_A0:
   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;		/* unit begin */
   RCregister_PC++;
   if (rom[RCregister_PC] != 0xEC) {
      goto opLSLe_A1;		/* no more, do last one */
   }

   RCregister_A <<= 1;
   RCregister_PC++;

 opLSLe_A1:
   RCregister_A &= 0xFFF;

//opLSLe_A2:
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

#if 0				/* non-multiple-consecutive-LSLe's version */
   temp_word = 0x0CEC;		/* data register */

   RCcmp_new = temp_word;	/* magic value */
   SETA0 (RCregister_A);	/* back up bit0 */
   RCcmp_old = RCregister_A;	/* store old acc */

   temp_word += RCregister_A;	/* add to acc */
   SETFC (temp_word);		/* store carry flag */
   RCregister_A <<= 1;		/* add regA to itself */
   RCregister_A &= 0xFFF;	/* toss excess bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;
#endif

 opLSLe_B_AA:
   temp_word = 0x0CEC;		/* data register */

   RCcmp_new = temp_word;	/* magic value */
   SETA0 (RCregister_A);	/* back up bit0 */
   RCcmp_old = RCregister_B;	/* store old acc */

   temp_word += RCregister_B;	/* add to acc */
   SETFC (temp_word);		/* store carry flag */
   RCregister_B <<= 1;		/* add regA to itself */
   RCregister_B &= 0xFFF;	/* toss excess bits */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_AA;

 opLSLf_A_AA:
 opLSLf_AA_AA:
 opLSLf_BB_AA:
   //UNFINISHED ("opLSLf 1\n");
   // added code for opcode missing from waroftheworlds
   RCregister_PC++;
   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);

   RCcmp_old = RCregister_A;
   temp_word += RCregister_A;

   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;
   // end of added code
   jumpCineRet_AA;

 opLSLf_B_AA:
   //UNFINISHED ("opLSLf 2\n");

   // added code for opcode missing from waroftheworlds
   /* ??; right shift pure; fill new bit with zero. */
   temp_word = 0x0BEB;

   RCregister_PC++;

   RCcmp_new = temp_word;
   SETA0 (RCregister_B);       // OR Register_A ??? check against Mame?
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);

   RCregister_B <<= 1;
   // end of added code
   jumpCineRet_AA;

 opASRe_A_AA:
 opASRe_AA_AA:
 opASRe_BB_AA:
   /* agh! I dislike these silly 12bit processors :P */

   temp_word = 0xDED;

   RCregister_PC++;

   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opASRe_A0;
   }
   RCcmp_new = temp_word;

   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;

   RCregister_A <<= 4;		/* get sign bit */
   RCregister_A = SAR16 (RCregister_A, 5);
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opASRe_A0:
   /* multiple ASRe's ... handle 'em in a batch, for efficiency */

   RCregister_A <<= 4;
   RCregister_A = SAR16 (RCregister_A, 1);

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opASRe_A1;
   }
   /* end of unit */
   RCregister_A = SAR16 (RCregister_A, 1);
   RCregister_PC++;

 opASRe_A1:
   /* no more multiples left; finish off. */
   RCregister_A >>= 4;

//opASRe_A2:

   /* shift once with flags */
   RCcmp_new = temp_word;

   SETA0 (RCregister_A);	/* store bit0 */
   SETFC (RCregister_A);

   RCcmp_old = RCregister_A;

   RCregister_A <<= 4;		/* get sign bit */
   RCregister_A = SAR16 (RCregister_A, 5);
   RCregister_A &= 0xFFF;

   jumpCineRet_AA;

 opASRe_B_AA:
   RCregister_PC++;

   if ((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57)) {
      goto opASRe_B0;		/* another one follows, do multiples */
   }

   RCcmp_new = 0x0DED;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRe_B0:
   RCregister_B <<= 4;		/* get sign bit */

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;
   if (!((rom[RCregister_PC] == 0xED) && (rom[RCregister_PC + 1] == 0x57))) {
      goto opASRe_B1;
   }

   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_PC += 2;

 opASRe_B1:
   RCregister_B >>= 4;		/* fix register */

//opASRe_B2:
   RCcmp_new = 0x0DED;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_B;

   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRf_A_AA:
 opASRf_AA_AA:
 opASRf_BB_AA:
   UNFINISHED ("opASRf 1\n");
   jumpCineRet_AA;

 opASRf_B_AA:
   UNFINISHED ("opASRf 2\n");
   jumpCineRet_AA;

 opASRDe_A_AA:
 opASRDe_AA_AA:
 opASRDe_BB_AA:
   /* Arithmetic shift right of D (A+B) .. B is high (sign bits). divide by
      2, but leave the sign bit the same. (ie: 1010 -> 1001) */
   temp_word = 0x0EEE;
   RCregister_PC++;

   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opASRDe_A0;		/* multiples, do the batch */
   }

   RCcmp_new = temp_word;	/* save new acc value */
   SETA0 (RCregister_A & 0xFF);	/* save old accA bit0 */
   RCcmp_old = RCregister_A;	/* save old acc */

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 4;
   RCregister_B <<= 4;

   temp_word_2 = (RCregister_B >> 4) << 15;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_A >>= 4;

   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

 opASRDe_A0:
   RCregister_A <<= 4;
   RCregister_B <<= 4;

   temp_word_2 = (RCregister_B >> 4) << 15;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;

   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more, do last one */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opASRDe_A1;		/* no more */
   }

   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;

   RCregister_PC++;

 opASRDe_A1:			/* do last shift with flags */
   RCregister_A >>= 4;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A & 0xFF);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 4;
   temp_word_2 = (RCregister_B & 0x01) << 15;
   RCregister_B = SAR16 (RCregister_B, 1);
   RCregister_A = (RCregister_A >> 1) | temp_word_2;
   RCregister_A >>= 4;

   RCregister_B &= 0xFFF;
   jumpCineRet_AA;

 opASRDe_B_AA:
   RCregister_PC++;
   temp_word = 0x0EEE;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A & 0xFF);
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);
   RCregister_B <<= 4;
   RCregister_B = SAR16 (RCregister_B, 5);
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opASRDf_A_AA:
 opASRDf_AA_AA:
 opASRDf_BB_AA:
   //UNFINISHED ("opASRDf 1\n");
   // code added for cosmicchasm
   temp_word = 0xFFF;

   RCregister_PC++;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 4;		/* get sign bit */
   RCregister_A = SAR16 (RCregister_A, 5);
   //RCregister_A >>= 1;               // need ASR not LSR
   RCregister_A &= 0xFFF;

   RCregister_B <<= 4;		/* get sign bit */
   RCregister_B = SAR16 (RCregister_B, 5);
   //RCregister_B >>= 1;               // need ASR not LSR
   RCregister_B &= 0xFFF;
   // end of added code
   jumpCineRet_AA;

 opASRDf_B_AA:
   //UNFINISHED ("opASRDf 2\n");
   // code added for cosmicchasm
   RCregister_PC++;

   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);

   RCregister_B <<= 4;		/* get sign bit */
   RCregister_B = SAR16 (RCregister_B, 5);
   //RCregister_B >>= 1;               // need ASR not LSR
   RCregister_B &= 0xFFF;
   // end of added code
   jumpCineRet_AA;

 opLSLDe_A_AA:
 opLSLDe_AA_AA:
 opLSLDe_BB_AA:
   /* LSLDe -- Left shift through both accumulators; lossy in middle. */

   temp_word = 0x0FEF;

   RCregister_PC++;
   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opLSLDe_A0;		/* multiples.. go to it. */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);
   RCregister_A <<= 1;		/* logical shift left */
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDe_A0:
   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;
   if (!(rom[RCregister_PC] == (temp_word & 0xFF))) {
      goto opLSLDe_A1;		/* nope, go do last one */
   }

   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;

 opLSLDe_A1:
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);
   RCregister_A <<= 1;		/* logical shift left */
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDe_B_AA:
   UNFINISHED ("opLSLD 1\n");
   jumpCineRet_AA;

 opLSLDf_A_AA:
 opLSLDf_AA_AA:
 opLSLDf_BB_AA:
   /* LSLDf */

   temp_word = 0xFFF;

   RCregister_PC++;
   if (rom[RCregister_PC] == (temp_word & 0xFF)) {
      goto opLSLDf_A0;		/* do multiple batches */
   }

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDf_A0:

   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;		/* unit begin */
   RCregister_B <<= 1;
   RCregister_PC++;
   if (rom[RCregister_PC] != (temp_word & 0xFF)) {
      goto opLSLDf_A1;
   }
   /* unit end */
   RCregister_A <<= 1;
   RCregister_B <<= 1;

   RCregister_PC++;

 opLSLDf_A1:
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   SETFC (RCregister_A);
   RCcmp_old = RCregister_A;

   temp_word += RCregister_A;
   SETFC (temp_word);

   RCregister_A <<= 1;
   RCregister_A &= 0xFFF;

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opLSLDf_B_AA:			/* not 'the same' as the A->AA version above */

   RCregister_PC++;

   temp_word = 0xFFF;
   RCcmp_new = temp_word;
   SETA0 (RCregister_A);
   RCcmp_old = RCregister_B;

   temp_word += RCregister_B;
   SETFC (temp_word);

   RCregister_B <<= 1;
   RCregister_B &= 0xFFF;

   jumpCineRet_AA;

 opJMP_A_A:
 opJMP_AA_A:
 opJMP_BB_A:
   /* simple jump; change PC and continue.. */

   /* Use 0xF000 so as to keep the current page, since it may well have been
      changed with JPP. */
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up new PC */
   jumpCineRet_A;

 opJMP_B_BB:
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up new PC */
   jumpCineRet_BB;

//opJEI_A_A:
 opJEI_AA_A:
 opJEI_BB_A:

   if (!(ioOutputs & 0x80)) {
      goto opja1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_A_A;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_A;

 opja1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_A_A;
   }

   RCregister_PC++;
   jumpCineRet_A;

 opJEI_B_BB:

   if (!(ioOutputs & 0x80)) {
      goto opjbb1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_BB;

 opjbb1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;
   jumpCineRet_BB;

 opJEI_A_B:
 opJEI_AA_B:
 opJEI_BB_B:

   if (!(ioOutputs & 0x80)) {
      goto opjb1;
   }

   if ((RCFromX - JoyX) > 0) {
      goto opJMP_A_B;
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_B;

 opjb1:

   if ((RCFromX - JoyY) > 0) {
      goto opJMP_A_B;
   }

   RCregister_PC++;
   jumpCineRet_B;

 opJMI_A_A:
   /* previous instruction was not an ACC instruction, nor was the
      instruction twice back a USB, therefore minus flag test the current
      A-reg */

   /* negative acc? */
   if (RCregister_A & 0x800) {
      goto opJMP_A_A;		/* yes -- do jump */
   }

   RCregister_PC++;		/* increment PC */
   jumpCineRet_A;

 opJMI_AA_A:
   /* previous acc negative? Jump if so... */
   if (RCcmp_old & 0x800) {
      goto opJMP_AA_A;
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJMI_BB_A:
   if (RCregister_B & 0x800) {
      goto opJMP_BB_A;
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJMI_B_BB:
   if (RCregister_A & 0x800) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJLT_A_A:
 opJLT_AA_A:
 opJLT_BB_A:
   /* jump if old acc equals new acc */

   if (RCcmp_new < RCcmp_old) {
      goto opJMP_A_A;
   }

   RCregister_PC++;
   jumpCineRet_A;

 opJLT_B_BB:
   if (RCcmp_new < RCcmp_old) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJEQ_A_A:
 opJEQ_AA_A:
 opJEQ_BB_A:
   /* jump if equal */

   if (RCcmp_new == RCcmp_old) {
      goto opJMP_A_A;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opJEQ_B_BB:

   if (RCcmp_new == RCcmp_old) {
      goto opJMP_B_BB;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opJA0_A_A:
 opJA0_AA_A:
 opJA0_BB_A:

   if (RCacc_a0 & 0x01) {
      goto opJMP_A_A;
   }

   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opJA0_B_BB:
   if (RCacc_a0 & 0x01) {
      goto opJMP_B_BB;
   }

   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opJNC_A_A:
 opJNC_AA_A:
 opJNC_BB_A:

   if (!(GETFC () & 0xF0)) {
      goto opJMP_A_A;		/* no carry, so jump */
   }
   RCregister_PC++;
   jumpCineRet_A;

 opJNC_B_BB:
   if (!(GETFC () & 0xF0)) {
      goto opJMP_B_BB;		/* no carry, so jump */
   }
   RCregister_PC++;
   jumpCineRet_BB;

 opJDR_A_A:
 opJDR_AA_A:
 opJDR_BB_A:
   /*
    * ; Calculate number of cycles executed since
    * ; last 'VDR' instruction, add two and use as
    * ; cycle count, never branch
    */
   RCregister_PC++;
   jumpCineRet_A;

 opJDR_B_BB:
   /*
    * ; Calculate number of cycles executed since
    * ; last 'VDR' instruction, add two and use as
    * ; cycle count, never branch
    */
   RCregister_PC++;
   jumpCineRet_BB;

 opNOP_A_A:
 opNOP_AA_A:
 opNOP_BB_A:
   RCregister_PC++;
   jumpCineRet_A;

 opNOP_B_BB:
   RCregister_PC++;
   jumpCineRet_BB;

 opJPP32_A_B:
 opJPP32_AA_B:
 opJPP32_BB_B:
   /* ; 00 = Offset 0000h ; 01 = Offset 1000h ; 02 = Offset 2000h ; 03 =
      Offset 3000h ; 04 = Offset 4000h ; 05 = Offset 5000h ; 06 = Offset
      6000h ; 07 = Offset 7000h */
   temp_word = (RCregister_P & 0x07) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP32_B_BB:
   temp_word = (RCregister_P & 0x07) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJPP16_A_B:
 opJPP16_AA_B:
 opJPP16_BB_B:
   /* ; 00 = Offset 0000h ; 01 = Offset 1000h ; 02 = Offset 2000h ; 03 = Offset 3000h */
   temp_word = (RCregister_P & 0x03) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP16_B_BB:
   temp_word = (RCregister_P & 0x03) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJMP_A_B:
   RCregister_PC = (RCregister_PC & 0xF000) + RCregister_J;	/* pick up
								   new PC */
   jumpCineRet_B;

 opJPP8_A_B:
 opJPP8_AA_B:
 opJPP8_BB_B:
   /* "long jump"; combine P and J to jump to a new far location (that can be
      more than 12 bits in address). After this jump, further jumps are local
      to this new page. */
   temp_word = ((RCregister_P & 0x03) - 1) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_B;

 opJPP8_B_BB:
   temp_word = ((RCregister_P & 0x03) - 1) << 12;	/* rom offset */
   RCregister_PC = RCregister_J + temp_word;
   jumpCineRet_BB;

 opJMI_A_B:
   if (RCregister_A & 0x800) {
      goto opJMP_A_B;
   }
   RCregister_PC++;
   jumpCineRet_B;

 opJMI_AA_B:
   UNFINISHED ("opJMI 3\n");
   jumpCineRet_B;

 opJMI_BB_B:
   UNFINISHED ("opJMI 4\n");
   jumpCineRet_B;

 opJLT_A_B:
 opJLT_AA_B:
 opJLT_BB_B:
   if (RCcmp_new < RCcmp_old) {
      goto opJMP_A_B;
   }
   RCregister_PC++;
   jumpCineRet_B;

 opJEQ_A_B:
 opJEQ_AA_B:
 opJEQ_BB_B:
   if (RCcmp_new == RCcmp_old) {
      goto opJMP_A_B;
   }
   RCregister_PC++;		/* bump PC */
   jumpCineRet_B;

 opJA0_A_B:
 opJA0_AA_B:
 opJA0_BB_B:

   if (GETA0 () & 0x01) {
      goto opJMP_A_B;
   }
   RCregister_PC++;

   jumpCineRet_B;

 opJNC_A_B:
 opJNC_AA_B:
 opJNC_BB_B:

   if (!(GETFC () & 0x0F0)) {
      goto opJMP_A_B;		/* if no carry, jump */
   }
   RCregister_PC++;

   jumpCineRet_B;

 opJDR_A_B:
 opJDR_AA_B:
 opJDR_BB_B:
   /* RCregister_PC++; */

   jumpCineRet_B;

   /* NOP */
 opNOP_A_B:
 opNOP_AA_B:
 opNOP_BB_B:
   RCregister_PC++;		/* NOP; bump PC only */
   jumpCineRet_B;

 opLLT_A_AA:
 opLLT_AA_AA:
 opLLT_BB_AA:
   RCregister_PC++;
   temp_byte = 0;

 opLLTa1:
   temp_word = RCregister_A >> 8;	/* RCregister_A's high bits */
   temp_word &= 0x0A;		/* only want PA11 and PA9 */

   if (!temp_word) {
      goto opLLTa2;		/* zero, no mismatch */
   }

   temp_word ^= 0x0A;		/* flip the bits */

   if (temp_word) {
      goto opLLTa4;		/* if not zero, mismatch found */
   }

 opLLTa2:
   temp_word = RCregister_B >> 8;	/* regB's top bits */
   temp_word &= 0x0A;		/* only want SA11 and SA9 */

   if (!temp_word) {
      goto opLLTa3;		/* if zero, no mismatch */
   }

   temp_word ^= 0x0A;		/* flip bits */

   if (temp_word) {
      goto opLLTa4;		/* if not zero, mismatch found */
   }

 opLLTa3:
   RCregister_A <<= 1;		/* shift regA */
   RCregister_B <<= 1;		/* shift regB */

   temp_byte++;
   if (temp_byte) {
      goto opLLTa1;		/* try again */
   }
   jumpCineRet_AA;

 opLLTa4:
   RCvgShiftLength = temp_byte;
   RCregister_A &= 0xFFF;
   RCregister_B &= 0xFFF;

//opLLTaErr:
   jumpCineRet_AA;

 opLLT_B_AA:
   UNFINISHED ("opLLT 1\n");
   jumpCineRet_AA;

 opVIN_A_A:
 opVIN_AA_A:
 opVIN_BB_A:
   /* set the starting address of a vector */

   RCFromX = RCregister_A & 0xFFF;	/* regA goes to x-coord */
   RCFromY = RCregister_B & 0xFFF;	/* regB goes to y-coord */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_A;

 opVIN_B_BB:

   RCFromX = RCregister_A & 0xFFF;	/* regA goes to x-coord */
   RCFromY = RCregister_B & 0xFFF;	/* regB goes to y-coord */

   RCregister_PC++;		/* bump PC */
   jumpCineRet_BB;

 opWAI_A_A:
 opWAI_AA_A:
 opWAI_BB_A:
   /* wait for a tick on the watchdog */
   bNewFrame = 1;
   RCregister_PC++;
   goto cineLoopExit;

 opWAI_B_BB:
   bNewFrame = 1;
   RCregister_PC++;
   goto cineLoopExit;

 opVDR_A_A:
 opVDR_AA_A:
 opVDR_BB_A:
   {
      int RCToX = SEX (RCregister_A & 0xFFF);
      int RCToY = SEX (RCregister_B & 0xFFF);

      RCFromX = SEX (RCFromX); RCFromY = SEX (RCFromY);

      /* figure out the vector */
      RCToX -= RCFromX; RCToX = SAR16 (RCToX, RCvgShiftLength); RCToX += RCFromX;
      RCToY -= RCFromY; RCToY = SAR16 (RCToY, RCvgShiftLength); RCToY += RCFromY;

#ifdef NEVER // currently, these are not initialised anywhere
      /* do orientation flipping, etc. */
      if (bFlipX) {
	 RCToX = sdwGameXSize - RCToX;
	 RCFromX = sdwGameXSize - RCFromX;
      }

      if (bFlipY) {
	 RCToY = sdwGameYSize - RCToY;
	 RCFromY = sdwGameYSize - RCFromY;
      }

      RCFromX += sdwXOffset;
      RCToX += sdwXOffset;

      RCFromY += sdwYOffset;
      RCToY += sdwYOffset;

      /* check real coords */
      if (bSwapXY) { 
	 temp_word = RCToY;
	 RCToY = RCToX;
	 RCToX = temp_word;
	 temp_word = RCFromY;
	 RCFromY = RCFromX;
	 RCFromX = temp_word;
      }
#endif
      
      /* render the line */
      CinemaVectorData (RCFromX, RCFromY, RCToX, RCToY, RCvgColour);
   }

   RCregister_PC++;
   jumpCineRet_A;

 opVDR_B_BB:
   UNFINISHED ("opVDR B 1\n");
   jumpCineRet_BB;

/* some code needs to be changed based on the machine or switches set.
 * Instead of getting disorganized, I'll put the extra dispatchers
 * here. The main dispatch loop jumps here, checks options, and
 * redispatches to the actual opcode handlers.
 */

/* JPP series of opcodes
 */
 tJPP_A_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_A_B;
      break;
   case 2:
      goto opJPP16_A_B;
      break;
   case 3:
      goto opJPP32_A_B;
      break;
   }

 tJPP_AA_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_AA_B;
      break;
   case 2:
      goto opJPP16_AA_B;
      break;
   case 3:
      goto opJPP32_AA_B;
      break;
   }

 tJPP_B_BB:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_B_BB;
      break;
   case 2:
      goto opJPP16_B_BB;
      break;
   case 3:
      goto opJPP32_B_BB;
      break;
   }

 tJPP_BB_B:
   /* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
   switch (ccpu_msize) {
   case 0:
   case 1:
      goto opJPP8_BB_B;
      break;
   case 2:
      goto opJPP16_BB_B;
      break;
   case 3:
      goto opJPP32_BB_B;
      break;
   }

/* JMI series of opcodes */

 tJMI_A_B:
   if (ccpu_jmi_dip) {
      goto opJMI_A_B;
   } else {
      goto opJEI_A_B;
   }

 tJMI_A_A:
   if (ccpu_jmi_dip) {
      goto opJMI_A_A;
   } else {
      goto opJEI_AA_B;
   }

 tJMI_AA_B:
   if (ccpu_jmi_dip) {
      goto opJMI_AA_B;
   } else {
      goto opJEI_AA_B;
   }

 tJMI_AA_A:
   if (ccpu_jmi_dip) {
      goto opJMI_AA_A;
   } else {
      goto opJEI_AA_A;
   }

 tJMI_B_BB1:
   if (ccpu_jmi_dip) {
      goto opJMI_B_BB;
   } else {
      goto opJEI_B_BB;
   }

 tJMI_B_BB2:
   if (ccpu_jmi_dip) {
      goto opJMI_B_BB;
   } else {
      goto opJEI_B_BB;
   }

 tJMI_BB_B:
   if (ccpu_jmi_dip) {
      goto opJMI_BB_B;
   } else {
      goto opJEI_BB_B;
   }

 tJMI_BB_A:
   if (ccpu_jmi_dip) {
      goto opJMI_BB_A;
   } else {
      goto opJEI_BB_A;
   }

/* OUT series of opcodes:
 * ccpu_monitor can be one of:
 * 1 -- 16-level colour
 * 2 -- 64-level colour
 * 3 -- War of the Worlds colour
 * other -- bi-level
 */
 tOUT_A_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_A_A;
      break;
   case 2:
      goto opOUT64_A_A;
      break;
   case 3:
      goto opOUTWW_A_A;
      break;
   default:
      goto opOUTbi_A_A;
   }

   goto opOUTbi_A_A;

 tOUT_AA_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_AA_A;
      break;
   case 2:
      goto opOUT64_AA_A;
      break;
   case 3:
      goto opOUTWW_AA_A;
      break;
   default:
      goto opOUTbi_AA_A;
   }

   goto opOUTbi_A_A;

 tOUT_B_BB:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_B_BB;
      break;
   case 2:
      goto opOUT64_B_BB;
      break;
   case 3:
      goto opOUTWW_B_BB;
      break;
   default:
      goto opOUTbi_B_BB;
   }

   goto opOUTbi_A_A;

 tOUT_BB_A:
   switch (ccpu_monitor) {
   case 1:
      goto opOUT16_BB_A;
      break;
   case 2:
      goto opOUT64_BB_A;
      break;
   case 3:
      goto opOUTWW_BB_A;
      break;
   default:
      goto opOUTbi_BB_A;
   }

   goto opOUTbi_A_A;


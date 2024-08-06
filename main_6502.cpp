// Program by Rubayat Areen to emulate a 6502 processor using C/C++

#include <stdio.h>
#include <stdlib.h>


using SByte = char;
using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;
using s32 = signed int;

struct Mem {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialize() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0;
        }
    }

    // read byte
    Byte operator[]( u32 Address ) const {
        // Address 
        return Data[Address];
    }
    
    // write 1 byte
    Byte& operator[]( u32 Address ) {
        // Address 
        return Data[Address];
    }

};

struct StatusFlags {
    Byte C : 1; // status flag 
    Byte Z : 1;
    Byte I : 1;
    Byte D : 1;
    Byte B : 1;
    Byte Unused : 1;
    Byte V : 1;
    Byte N : 1;

};

struct CPU {
    
    Word PC;           // program counter
    Byte SP;           // stack pointer


    Byte A, X, Y;      // registers

    union {
        Byte PS;
        StatusFlags Flag;

    };
    
    void Reset( Mem& memory ) {
        PC = 0xFFFC;
        SP = 0xFF;
        Flag.C = Flag.Z = Flag.I = Flag.D = Flag.B = Flag.V = Flag.N = Flag.D = 0;
        A = X = Y = 0;
        memory.Initialize();
    }

    Byte FetchByte( u32& Cycles, const Mem& memory ) {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;

    }

    SByte FetchSByte( u32& Cycles, const Mem& memory ) {
        return FetchByte(Cycles, memory);

    }

    Word FetchWord ( u32& Cycles, const Mem& memory )
    {
        // 6502 is little endian
        Word Data = memory[PC];
        PC++;

        Data |= (memory[PC] << 8);
        PC++;
        Cycles-=2;


        return Data;
    }


    Byte ReadByteFromZeroPage( u32& Cycles, Byte Address,  Mem& memory ) 
    {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }

    Byte ReadByte( u32& Cycles, Word Address, const Mem& memory ) 
    {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }

    Word ReadWord( u32& Cycles, Word Address, const Mem& memory ) 
    {
        Byte LoByte = ReadByte(Cycles, Address, memory);
        Byte HiByte = ReadByte(Cycles, Address + 1, memory);
        return LoByte | (HiByte << 8);
    }

    // write 1 byte to memory
    void WriteByte( Byte Value, u32& Cycles, Word Address, Mem& memory )
    {
        memory[Address] = Value;
        Cycles--;
    }

    // write 2 bytes to memory
    void WriteWord(Word Value, u32& Cycles, Word Address, Mem& memory) 
    {
        memory[Address]   = Value & 0xFF;
        memory[Address+1] = (Value >> 8);
        Cycles -= 2;
    }

    Word SPToAddress() const
    {
        return 0x100 | SP;
    }

    void PushWordToStack(u32& Cycles, Mem& memory, Word Value)
    {
        Word PCMinusOne = PC - 1;
        WriteByte(Value >> 8, Cycles, SPToAddress(), memory);
        SP--;
        WriteByte(Value & 0xFF, Cycles, SPToAddress(), memory);
        SP--; 
    }

    void PushPCMinusOneToStack(u32& Cycles, Mem& memory)
    {
        PushWordToStack(Cycles, memory, PC-1); 
    }

    void PushPCPlusOneToStack(u32& Cycles, Mem& memory)
    {
        PushWordToStack(Cycles, memory, PC+1); 
    }

    void PushPCToStack(u32& Cycles, Mem& memory)
    {
        PushWordToStack(Cycles, memory, PC);
    }

    void PushByteOnToStack(u32& Cycles, Byte Value, Mem& memory)
    {
        const Word SPWord = SPToAddress();
        memory[SPWord] = Value;
        Cycles--;
        SP--;
        Cycles--;
    }
    
    Byte PopByteFromStack (u32& Cycles, Mem& memory)
    {
        SP++;
        Cycles--;
        const Word SPWord = SPToAddress();
        Byte Value = memory[SPWord];
        Cycles--;
        return Value;

    }


    Word PopWordFromStack (u32& Cycles, Mem& memory)
    {
        Word ValueFromStack = ReadWord(Cycles, SPToAddress()+1, memory);
        SP += 2;
        Cycles--;
        return ValueFromStack;

    }

    static constexpr Byte 
        NegativeFlagBit =  0b10000000,
        OverflowFlagBit =  0b01000000,
        ZeroBit = 0b00000001, 
        BreakFlagBit = 0b00010000,
        UnusedFlagBit = 0b00100000;

    // opcodes
    static constexpr Byte
		//LDA
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_LDA_ABS = 0xAD,
		INS_LDA_ABSX = 0xBD,
		INS_LDA_ABSY = 0xB9,
		INS_LDA_INDX = 0xA1,
		INS_LDA_INDY = 0xB1,
		//LDX
		INS_LDX_IM = 0xA2,
		INS_LDX_ZP = 0xA6,
		INS_LDX_ZPY = 0xB6,
		INS_LDX_ABS = 0xAE,
		INS_LDX_ABSY = 0xBE,
		//LDY
		INS_LDY_IM = 0xA0,
		INS_LDY_ZP = 0xA4,
		INS_LDY_ZPX = 0xB4,
		INS_LDY_ABS = 0xAC,
		INS_LDY_ABSX = 0xBC,
		//STA
		INS_STA_ZP = 0x85,
		INS_STA_ZPX = 0x95,
		INS_STA_ABS = 0x8D,
		INS_STA_ABSX = 0x9D,
		INS_STA_ABSY = 0x99,
		INS_STA_INDX = 0x81,
		INS_STA_INDY = 0x91,
		//STX
		INS_STX_ZP = 0x86,
		INS_STX_ZPY = 0x96,
		INS_STX_ABS = 0x8E,
		//STY
		INS_STY_ZP = 0x84,
		INS_STY_ZPX = 0x94,
		INS_STY_ABS = 0x8C,

		INS_TSX = 0xBA,
		INS_TXS = 0x9A,
		INS_PHA = 0x48,
		INS_PLA = 0x68,
		INS_PHP = 0x08,
		INS_PLP = 0x28,

		INS_JMP_ABS = 0x4C,
		INS_JMP_IND = 0x6C,
		INS_JSR = 0x20,
		INS_RTS = 0x60,
		
		//Logical Ops

		//AND
		INS_AND_IM = 0x29,
		INS_AND_ZP = 0x25,
		INS_AND_ZPX = 0x35,
		INS_AND_ABS = 0x2D,
		INS_AND_ABSX = 0x3D,
		INS_AND_ABSY = 0x39,
		INS_AND_INDX = 0x21,
		INS_AND_INDY = 0x31,

		//OR
		INS_ORA_IM = 0x09,
		INS_ORA_ZP = 0x05,
		INS_ORA_ZPX = 0x15,
		INS_ORA_ABS = 0x0D,
		INS_ORA_ABSX = 0x1D,
		INS_ORA_ABSY = 0x19,
		INS_ORA_INDX = 0x01,
		INS_ORA_INDY = 0x11,

		//EOR
		INS_EOR_IM = 0x49,
		INS_EOR_ZP  = 0x45,
		INS_EOR_ZPX = 0x55,
		INS_EOR_ABS = 0x4D,
		INS_EOR_ABSX = 0x5D,
		INS_EOR_ABSY = 0x59,
		INS_EOR_INDX = 0x41,
		INS_EOR_INDY = 0x51,

		//BIT
		INS_BIT_ZP = 0x24,
		INS_BIT_ABS = 0x2C,

		//Transfer Registers
		INS_TAX = 0xAA,
		INS_TAY = 0xA8,
		INS_TXA = 0x8A,
		INS_TYA = 0x98,

		//Increments, Decrements
		INS_INX = 0xE8,
		INS_INY = 0xC8,
		INS_DEY = 0x88,
		INS_DEX = 0xCA,
		INS_DEC_ZP = 0xC6,
		INS_DEC_ZPX = 0xD6,
		INS_DEC_ABS = 0xCE,
		INS_DEC_ABSX = 0xDE,
		INS_INC_ZP = 0xE6,
		INS_INC_ZPX = 0xF6,
		INS_INC_ABS = 0xEE,
		INS_INC_ABSX = 0xFE,

		//branches
		INS_BEQ = 0xF0,
		INS_BNE = 0xD0,
		INS_BCS = 0xB0,
		INS_BCC = 0x90,
		INS_BMI = 0x30,
		INS_BPL = 0x10,
		INS_BVC = 0x50,
		INS_BVS = 0x70,

		//status flag changes
		INS_CLC = 0x18,
		INS_SEC = 0x38,
		INS_CLD = 0xD8,
		INS_SED = 0xF8,
		INS_CLI = 0x58,
		INS_SEI = 0x78,
		INS_CLV = 0xB8,

		//Arithmetic
		INS_ADC_IM = 0x69,
		INS_ADC_ZP = 0x65,
		INS_ADC_ZPX = 0x75,
		INS_ADC_ABS = 0x6D,
		INS_ADC_ABSX = 0x7D,
		INS_ADC_ABSY = 0x79,
		INS_ADC_INDX = 0x61,
		INS_ADC_INDY = 0x71,

		INS_SBC = 0xE9,
		INS_SBC_ABS = 0xED,
		INS_SBC_ZP = 0xE5,
		INS_SBC_ZPX = 0xF5,
		INS_SBC_ABSX = 0xFD,
		INS_SBC_ABSY = 0xF9,
		INS_SBC_INDX = 0xE1,
		INS_SBC_INDY = 0xF1,

		// Register Comparison
		INS_CMP = 0xC9,
		INS_CMP_ZP = 0xC5,
		INS_CMP_ZPX = 0xD5,
		INS_CMP_ABS = 0xCD,
		INS_CMP_ABSX = 0xDD,
		INS_CMP_ABSY = 0xD9,
		INS_CMP_INDX = 0xC1,
		INS_CMP_INDY = 0xD1,

		INS_CPX = 0xE0,
		INS_CPY = 0xC0,
		INS_CPX_ZP = 0xE4,
		INS_CPY_ZP = 0xC4,
		INS_CPX_ABS = 0xEC,
		INS_CPY_ABS = 0xCC,

		// shifts
		INS_ASL = 0x0A,
		INS_ASL_ZP = 0x06,
		INS_ASL_ZPX = 0x16,
		INS_ASL_ABS = 0x0E,
		INS_ASL_ABSX = 0x1E,

		INS_LSR = 0x4A,
		INS_LSR_ZP = 0x46,
		INS_LSR_ZPX = 0x56,
		INS_LSR_ABS = 0x4E,
		INS_LSR_ABSX = 0x5E,

		INS_ROL = 0x2A,
		INS_ROL_ZP = 0x26,
		INS_ROL_ZPX = 0x36,
		INS_ROL_ABS = 0x2E,
		INS_ROL_ABSX = 0x3E,

		INS_ROR = 0x6A,
		INS_ROR_ZP = 0x66,
		INS_ROR_ZPX = 0x76,
		INS_ROR_ABS = 0x6E,
		INS_ROR_ABSX = 0x7E,

		//misc
		INS_NOP = 0xEA,
		INS_BRK = 0x00,
		INS_RTI = 0x40
		;

    void SetZeroAndNegativeFlags(Byte Register)
    {
        Flag.Z = (Register == 0);
        Flag.N = (Register & 0b10000000) > 0;
    }

    Word AddrZeroPage( u32& Cycles, const Mem& memory)
    {
        Byte ZeroPageAddr = FetchByte ( Cycles, memory );
        return ZeroPageAddr;
    }

    Word AddrZeroPageX( u32& Cycles, const Mem& memory)
    {
        Byte ZeroPageAddr = FetchByte ( Cycles, memory );
        ZeroPageAddr += X;
        Cycles--;
        return ZeroPageAddr;
    }

    Word AddrZeroPageY( u32& Cycles, const Mem& memory)
    {
        Byte ZeroPageAddr = FetchByte ( Cycles, memory );
        ZeroPageAddr += Y;
        Cycles--;
        return ZeroPageAddr;
    }

    Word AddrAbsolute( u32& Cycles, const Mem& memory)
    {
        Word AbsAddress = FetchWord(Cycles, memory);
        return AbsAddress;
    }

    Word AddrAbsoluteX( u32& Cycles, const Mem& memory)
    {
        Word AbsAddress = FetchWord(Cycles, memory);
        Word AbsAddressX = AbsAddress + X;
        if ((AbsAddressX & 0xFF00) !=  (AbsAddress & 0xFF00))
        {
            Cycles--;
        }
        return AbsAddressX;
    }

    Word AddrAbsoluteX_5( u32& Cycles, const Mem& memory)
    {
        Word AbsAddress = FetchWord(Cycles, memory);
        Word AbsAddressX = AbsAddress + X;
        Cycles--;
        return AbsAddressX;
    }
    
    Word AddrAbsoluteY( u32& Cycles, const Mem& memory)
    {
        Word AbsAddress = FetchWord(Cycles, memory);
        Word AbsAddressY = AbsAddress + Y;
        if ((AbsAddressY & 0xFF00) !=  (AbsAddress & 0xFF00))
        {
            Cycles--;
        }
        return AbsAddressY;
    }

    Word AddrAbsoluteY_5( u32& Cycles, const Mem& memory)
    {
        Word AbsAddress = FetchWord(Cycles, memory);
        Word AbsAddressY = AbsAddress + Y;
        Cycles--;
        return AbsAddressY;
    }

    Word AddrIndirectX( u32& Cycles, const Mem& memory)
    {
        Byte ZPAddress = FetchByte( Cycles, memory );
        ZPAddress += X;
        Cycles--;
        Word EffectiveAddr = ReadWord(Cycles, ZPAddress, memory);
        return EffectiveAddr;
    }

    Word AddrIndirectY( u32& Cycles, const Mem& memory)
    {
        Byte ZPAddress = FetchByte( Cycles, memory );
        Word EffectiveAddr = ReadWord( Cycles, ZPAddress, memory);
        Word EffectiveAddrY = EffectiveAddr + Y;
        if ((EffectiveAddrY & 0xFF00) !=  (EffectiveAddr & 0xFF00))
        {
            Cycles--;
        }
        return EffectiveAddrY;
    }

    Word AddrIndirectY_6( u32& Cycles, const Mem& memory)
    {
        Byte ZPAddress = FetchByte( Cycles, memory );
        Word EffectiveAddr = ReadWord( Cycles, ZPAddress, memory);
        Word EffectiveAddrY = EffectiveAddr + Y;
        Cycles--;
        return EffectiveAddrY;
    }


    void Execute( u32 Cycles, Mem& memory )
    {

        // Load a Register with a value from the memory address

        auto LoadRegister = [&Cycles, &memory, this](Word Address, Byte& Register)
        {
            Register = ReadByte(Cycles, Address, memory);
            SetZeroAndNegativeFlags(Register);
        };

        auto And = [&Cycles, &memory, this](Word Address)
        {
            A &= ReadByte(Cycles, Address, memory);
            SetZeroAndNegativeFlags(A);
        };

        auto Ora = [&Cycles, &memory, this](Word Address)
        {
            A |= ReadByte(Cycles, Address, memory);
            SetZeroAndNegativeFlags(A);
        };

        auto Eor = [&Cycles, &memory, this](Word Address)
        {
            A ^= ReadByte(Cycles, Address, memory);
            SetZeroAndNegativeFlags(A);
        };

        auto BranchIf = [&Cycles, &memory, this]( bool Test, bool Expected) 
        {
            Byte Offset = FetchSByte(Cycles, memory);
                if (Test == Expected)
                {
                    const Word PCOld = PC;
                    PC += Offset;
                    Cycles--;
                    const bool PageChanged = (PC >> 8) != (PCOld >> 8);
                    if (PageChanged)
                    {
                        Cycles--;
                    }
                }
        };


        auto ADC = [&Cycles, &memory, this](Byte Operand) 
        {
            const bool AreSignBitsTheSame = !((A ^ Operand) & NegativeFlagBit);
            Word Sum = A;
            Sum += Operand;
            Sum += Flag.C;
            A = Sum & 0xFF;
            SetZeroAndNegativeFlags(A);
            Flag.C = Sum > 0xFF;
            Flag.V = AreSignBitsTheSame && (((A & Operand) & NegativeFlagBit) > 0);
        };

        auto SBC = [&ADC](Byte Operand) 
        {
            ADC(~Operand);
        };

        auto ASL = [&Cycles, this](Byte Operand) -> Byte
        {
            Flag.C = (Operand & NegativeFlagBit) > 0;
            Byte Result = Operand << 1;
            SetZeroAndNegativeFlags(Result);
            Cycles--;
            return Result;
        };

        auto LSR = [&Cycles, this](Byte Operand) -> Byte
        {
            constexpr Byte BitZero = 0b00000001;
            Flag.C = (Operand & BitZero) > 0;
            Byte Result = Operand >> 1;
            SetZeroAndNegativeFlags(Result);
            Cycles--;
            return Result;
        };

        auto ROL = [&Cycles, this]( Byte Operand ) -> Byte
        {
            Byte NewBit0 = Flag.C ? ZeroBit : 0;
            Flag.C = (Operand & NegativeFlagBit) > 0;
            Operand = Operand << 1;
            Operand |= NewBit0;
            SetZeroAndNegativeFlags(Operand);
            Cycles--;
            return Operand;
        };

        auto ROR = [&Cycles, this]( Byte Operand ) -> Byte
        {
            bool OldBit0 = (Operand & ZeroBit) > 0;
            Operand = Operand >> 1;
            if (Flag.C)
            {
                Operand |= NegativeFlagBit;
            }
            Cycles--;
            Flag.C = OldBit0;
            SetZeroAndNegativeFlags(Operand);
        };

        auto RegisterCompare = [&Cycles, &memory, this](Word Operand, Byte RegisterValue)
        {
            Byte Temp = RegisterValue - Operand;
            Flag.N = (Temp & NegativeFlagBit) > 0;
            Flag.Z = A == Operand;
            Flag.C = A >= Operand;
        };

        auto PushPSToStack = [&Cycles, &memory, this]()
        {
            Byte PSStack = PS | BreakFlagBit | UnusedFlagBit;
            PushByteOnToStack(Cycles, PSStack, memory);
        };

        auto PopPSFromStack = [&Cycles, &memory, this]()
        {
            PS = PopByteFromStack(Cycles, memory);
            Flag.B = false;
            Flag.Unused = false;
        };

        while(Cycles > 0)
        {
            Byte Ins = FetchByte(Cycles, memory);
            switch ( Ins )
            {
            case INS_LDA_IM:
            {
                A = FetchByte ( Cycles, memory );
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_LDX_IM:
            {
                X = FetchByte ( Cycles, memory );
                SetZeroAndNegativeFlags(X);
            } break;
            case INS_LDY_IM:
            {
                Y = FetchByte ( Cycles, memory );
                SetZeroAndNegativeFlags(Y);
            } break;
            case INS_LDA_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                LoadRegister( Address, A);

            } break;
            case INS_LDX_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                LoadRegister( Address, X);
            } break;
            case INS_LDX_ZPY:
            {
                Word Address = AddrZeroPageY(Cycles, memory);
                LoadRegister( Address, X);

            } break;
            case INS_LDY_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                LoadRegister( Address, Y);

            } break;
            case INS_LDA_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                LoadRegister( Address, A);
            } break;
            case INS_LDY_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                LoadRegister( Address, Y);
            } break;
            case INS_LDA_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                LoadRegister( Address, A);
            } break;
            case INS_LDX_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                LoadRegister( Address, X);
            } break;
            case INS_LDY_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                LoadRegister( Address, Y);
            } break;
            case INS_LDA_ABSX:
            {
                Word Address = AddrAbsoluteX( Cycles, memory);
                LoadRegister( Address, A);
                
            } break;
            case INS_LDY_ABSX:
            {
                Word Address = AddrAbsoluteX( Cycles, memory);
                LoadRegister( Address, Y);
                
            } break;
            case INS_LDA_ABSY:
            {
                Word Address = AddrAbsoluteY( Cycles, memory);
                LoadRegister( Address, A);
                
            } break;
            case INS_LDX_ABSY:
            {
                Word Address = AddrAbsoluteY( Cycles, memory);
                LoadRegister( Address, X);
                
            } break;
            case INS_LDA_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                LoadRegister( Address, A);
            } break;
            case INS_LDA_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                LoadRegister( Address, A);
            } break;
            case INS_STA_ZP:
            {
                Word Address = AddrZeroPage( Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STX_ZP:
            {
                Word Address = AddrZeroPage( Cycles, memory);
                WriteByte(X, Cycles, Address, memory);

            } break;
            case INS_STX_ZPY:
            {
                Word Address = AddrZeroPageY( Cycles, memory);
                WriteByte(X, Cycles, Address, memory);

            } break;
            case INS_STY_ZP:
            {
                Word Address = AddrZeroPage( Cycles, memory);
                WriteByte(Y, Cycles, Address, memory);

            } break;
            case INS_STA_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STX_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory);
                WriteByte(X, Cycles, Address, memory);

            } break;
            case INS_STY_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory);
                WriteByte(Y, Cycles, Address, memory);

            } break;
            case INS_STA_ZPX:
            {
                Word Address = AddrZeroPageX( Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STY_ZPX:
            {
                Word Address = AddrZeroPageX( Cycles, memory);
                WriteByte(Y, Cycles, Address, memory);

            } break;
            case INS_STA_ABSX:
            {
                Word Address = AddrAbsoluteX_5( Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STA_ABSY:
            {
                Word Address = AddrAbsoluteY_5( Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STA_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_STA_INDY:
            {
                Word Address = AddrIndirectY_6(Cycles, memory);
                WriteByte(A, Cycles, Address, memory);

            } break;
            case INS_JSR:
            {
                Word SubAddr = FetchWord(Cycles, memory);
                PushPCMinusOneToStack( Cycles, memory);
                PC = SubAddr;
                Cycles--;
            } break;
            case INS_RTS:
            {
                Word ReturnAddress = PopWordFromStack(Cycles, memory);
                PC = ReturnAddress + 1;
                Cycles -= 2;
            } break;
            case INS_JMP_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory );
                PC = Address; 
            } break;
            case INS_JMP_IND:
            {
                Word Address = AddrAbsolute( Cycles, memory );
                Address = ReadWord( Cycles, Address, memory );
                PC = Address; 
            } break;
            case INS_TSX:
            {
                X = SP;
                Cycles--;
                SetZeroAndNegativeFlags(X);

            } break;
            case INS_TXS:
            {
                SP = X;
                Cycles--;

            } break;
            case INS_PHA:
            {
                PushByteOnToStack(Cycles, A, memory);
            } break;
            case INS_PHP:
            {
                PushPSToStack();
            } break;
            case INS_PLA:
            {
                A = PopByteFromStack(Cycles, memory);
                SetZeroAndNegativeFlags(A);
                Cycles--;
            } break;
            case INS_PLP:
            {
                PopPSFromStack();
                Cycles--;
            } break;
            case INS_AND_IM:
            {
                A &= FetchByte(Cycles, memory);
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_ORA_IM:
            {
                A |= FetchByte(Cycles, memory);
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_EOR_IM:
            {
                A ^= FetchByte(Cycles, memory);
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_AND_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_ABSX:
            {
                Word Address = AddrAbsoluteX(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_ABSX:
            {
                Word Address = AddrAbsoluteX(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_ABSX:
            {
                Word Address = AddrAbsoluteX(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_ABSY:
            {
                Word Address = AddrAbsoluteY(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_ABSY:
            {
                Word Address = AddrAbsoluteY(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_ABSY:
            {
                Word Address = AddrAbsoluteY(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                Eor(Address);
            } break;
            case INS_AND_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                And(Address);
            } break;
            case INS_ORA_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                Ora(Address);
            } break;
            case INS_EOR_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                Eor(Address);
            } break;
            case INS_BIT_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Flag.Z = !(A & Value);
                Flag.N = (Value & NegativeFlagBit) != 0;
                Flag.V = (Value * OverflowFlagBit) != 0;

            } break;
            case INS_BIT_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Flag.Z = !(A & Value);
                Flag.N = (Value & NegativeFlagBit) != 0;
                Flag.V = (Value * OverflowFlagBit) != 0;

            } break;
            case INS_TAX:
            {
                X = A;
                Cycles--;
                SetZeroAndNegativeFlags(X);
            } break;
            case INS_TAY:
            {
                Y = A;
                Cycles--;
                SetZeroAndNegativeFlags(Y);
            } break;
            case INS_TXA:
            {
                A = X;
                Cycles--;
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_TYA:
            {
                A = Y;
                Cycles--;
                SetZeroAndNegativeFlags(A);
            } break;
            case INS_INX:
            {
                X++;
                Cycles--;
                SetZeroAndNegativeFlags(X);
            } break;
            case INS_INY:
            {
                Y++;
                Cycles--;
                SetZeroAndNegativeFlags(Y);
            } break;
            case INS_DEY:
            {
                Y--;
                Cycles--;
                SetZeroAndNegativeFlags(Y);
            } break;
            case INS_DEX:
            {
                X--;
                Cycles--;
                SetZeroAndNegativeFlags(Y);
            } break;
            case INS_DEC_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value--;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_DEC_ZPX:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Address += X;
                Cycles--;
                Byte Value = ReadByte(Cycles, Address, memory);
                Value--;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_DEC_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value--;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_DEC_ABSX:
            {
                Word Address = AddrAbsoluteX_5(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value++;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_INC_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value++;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_INC_ZPX:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Address += X;
                Cycles--;
                Byte Value = ReadByte(Cycles, Address, memory);
                Value++;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_INC_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value++;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_INC_ABSX:
            {
                Word Address = AddrAbsoluteX_5(Cycles, memory);
                Byte Value = ReadByte(Cycles, Address, memory);
                Value++;
                Cycles--;
                WriteByte(Value, Cycles, Address, memory);
                SetZeroAndNegativeFlags(Value);
            } break;
            case INS_BEQ:
            {   
                BranchIf(Flag.Z, true);
            } break;
            case INS_BNE:
            {   
                BranchIf(Flag.Z, false);
            } break;
            case INS_BCS:
            {   
                BranchIf(Flag.C, true);
            } break;
            case INS_BCC:
            {   
                BranchIf(Flag.C, false);
            } break;
            case INS_BMI:
            {   
                BranchIf(Flag.N, true);
            } break;
            case INS_BPL:
            {   
                BranchIf(Flag.N, false);
            } break;
            case INS_BVC:
            {   
                BranchIf(Flag.V, false);
            } break;
            case INS_BVS:
            {   
                BranchIf(Flag.V, true);
            } break;
            case INS_CLC:
            {
                Flag.C = false;
                Cycles--;
            } break;
            case INS_SEC:
            {
                Flag.C = true;
                Cycles--;
            } break;
            case INS_CLD:
            {
                Flag.D = false;
                Cycles--;
            } break;
            case INS_SED:
            {
                Flag.D = true;
                Cycles--;
            } break;
            case INS_CLI:
            {
                Flag.I = false;
                Cycles--;
            } break;
            case INS_CLV:
            {
                Flag.V = false;
                Cycles--;
            } break;
            case INS_SEI:
            {
                Flag.I = true;
                Cycles--;
            } break;
            case INS_NOP:
            {
                Cycles--;
            } break;
            case INS_ADC_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_IM:
            {
                Byte Operand = FetchByte(Cycles, memory);
                ADC(Operand);
            } break;
            case INS_ADC_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_ABSX:
            {
                Word Address = AddrAbsoluteX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_ABSY:
            {
                Word Address = AddrAbsoluteY(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_ADC_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                ADC(Operand);
            } break;
            case INS_CMP:
            {
                Byte Operand = FetchByte(Cycles, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CPX:
            {
                Byte Operand = FetchByte(Cycles, memory);
                RegisterCompare(Operand, X);
            } break;
            case INS_CPY:
            {
                Byte Operand = FetchByte(Cycles, memory);
                RegisterCompare(Operand, Y);
            } break;
            case INS_CMP_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CPX_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, X);
            } break;
            case INS_CPY_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, Y);
            } break;
            case INS_CMP_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CMP_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CPX_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, X);
            } break;
            case INS_CPY_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, Y);
            } break;
            case INS_CMP_ABSX:
            {
                Word Address = AddrAbsoluteX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CMP_ABSY:
            {
                Word Address = AddrAbsoluteY(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CMP_INDX:
            {
                Word Address = AddrIndirectX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_CMP_INDY:
            {
                Word Address = AddrIndirectY(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                RegisterCompare(Operand, A);
            } break;
            case INS_SBC:
            {
                Byte Operand = FetchByte( Cycles, memory );
                SBC( Operand );
            } break;
            case INS_SBC_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_ZP:
            {
                Word Address = AddrZeroPage( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_ZPX:
            {
                Word Address = AddrZeroPageX( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_ABSX:
            {
                Word Address = AddrAbsoluteX( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_ABSY:
            {
                Word Address = AddrAbsoluteY( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_INDX:
            {
                Word Address = AddrIndirectX( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_SBC_INDY:
            {
                Word Address = AddrIndirectY( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                SBC( Operand );
            } break;
            case INS_ASL:
            {
                A = ASL(A);
            } break;
            case INS_ASL_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ASL(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_ASL_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ASL(Operand);
                WriteByte(Result, Cycles, Address, memory );
            } break;
            case INS_ASL_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ASL( Operand );
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_ASL_ABSX:
            {
                Word Address = AddrAbsoluteX_5(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory );
                Byte Result = ASL(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_LSR:
            {
                A = LSR(A);
            } break;
            case INS_LSR_ZP:
            {
                Word Address = AddrZeroPage(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = LSR(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_LSR_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = LSR(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_LSR_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = LSR(Operand);
                WriteByte( Result, Cycles, Address, memory);
            } break;
            case INS_LSR_ABSX:
            {
                Word Address = AddrAbsoluteX_5( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                Byte Result = LSR( Operand );
                WriteByte( Result, Cycles, Address, memory );
            } break;
            case INS_ROL:
            {
                A = ROL(A);
            } break;
            case INS_ROL_ZP:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ROL(Operand);
                WriteByte( Result, Cycles, Address, memory);
            } break;
            case INS_ROL_ZPX:
            {
                Word Address = AddrZeroPageX( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                Byte Result = ROL( Operand );
                WriteByte( Result, Cycles, Address, memory );
            } break;
            case INS_ROL_ABS:
            {
                Word Address = AddrAbsolute( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                Byte Result = ROL( Operand );
                WriteByte( Result, Cycles, Address, memory );
            } break;
            case INS_ROL_ABSX:
            {
                Word Address = AddrAbsoluteX_5( Cycles, memory );
                Byte Operand = ReadByte( Cycles, Address, memory );
                Byte Result = ROL( Operand );
                WriteByte( Result, Cycles, Address, memory );
            } break;
            case INS_ROR:
            {
                A = ROR(A);
            } break; 
            case INS_ROR_ZP:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ROR(Operand);
                WriteByte( Result, Cycles, Address, memory);
            } break;
            case INS_ROR_ZPX:
            {
                Word Address = AddrZeroPageX(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ROR(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_ROR_ABS:
            {
                Word Address = AddrAbsolute(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ROR(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_ROR_ABSX:
            {
                Word Address = AddrAbsoluteX_5(Cycles, memory);
                Byte Operand = ReadByte(Cycles, Address, memory);
                Byte Result = ROR(Operand);
                WriteByte(Result, Cycles, Address, memory);
            } break;
            case INS_BRK:
            {
                PushPCPlusOneToStack(Cycles, memory);
                PushPSToStack();
                constexpr Word InterruptVector = 0xFFFE;
                PC = ReadWord(Cycles, InterruptVector, memory);
                Flag.B = true;
                Flag.I = true;
            } break;
            case INS_RTI:
            {
                PopPSFromStack();
                PC = PopWordFromStack( Cycles, memory );
            } break;
            default:
            {
               printf("Instruction not handled %d", Ins);
            } break;
            }
        } 

    }

};

int main()
{
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);
    return 0;
}
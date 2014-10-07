# This file handles outputting the various pieces of syntax that the
# asm generation process currently relies on a c++ header file to resolve:
# local and global labels, symbols, and references.
# Inline assembly (especially the size of that used with LLInt) does not
# work with all build toolchains (MSVC).

def thumb_func_param(name)
    if $useMacros
        "THUMB_FUNC_PARAM("+name+")"
    else
		#only IOS uses this macro
		""
    end
end

def inline_arm_function(name)
    if $useMacros
        "INLINE_ARM_FUNCTION("+name+")"
	else
		if $asmtool == "armasm"
			"CODE16\n"
		elsif $asmtool == "masm"
		    "ERROR - we shouldn't hit this case (inline_arm_function & masm)"
		else
			".thumb\n"+
			".thumb_func "+thumb_func_param(name)+"\n"
		end	
    end
end

def hide_symbol(name)
    if $useMacros
        "HIDE_SYMBOL("+name+")"
    else
		""
    end
end

def symbol_string(name)
    if $useMacros
        "SYMBOL_STRING("+name+")"
	else
		"_"+name
    end
end

def offline_asm_global_label(label)
    if $useMacros
        "OFFLINE_ASM_GLOBAL_LABEL("+label+")"
    else
		if $asmtool == "armasm"
			"\tEXPORT "+symbol_string(label)+"\n"+
			# hide_symbol(label)+"\n"+
			# "THUMB16\n"+
			#".thumb_func "+thumb_func_param(label)+"\n"+
			symbol_string(label)+" PROC\n"
		elsif $asmtool == "masm"
		    "%       public "+symbol_string(label)+"\n"
            symbol_string(label)+"   proc"
		else
			".globl "+symbol_string(label)+"\n"+
			hide_symbol(label)+"\n"+
			".thumb\n"+
			".thumb_func "+thumb_func_param(label)+"\n"+
			symbol_string(label)+":\n"
		end    
    end
end

def offline_asm_opcode_label(opcode)
    if $useMacros
        "OFFLINE_ASM_OPCODE_LABEL("+opcode+")"
    else
        offline_asm_global_label("llint_"+opcode)
    end
end

def offline_asm_local_label(label)
    if $useMacros
        "  OFFLINE_ASM_LOCAL_LABEL("+label+")"
    else
		if $asmtool == "armasm"
			local_label_string(label)+" PROC\n"
		elsif $asmtool == "masm"
		    local_label_string(label)+":\n"
		else
			local_label_string(label)+":\n"
		end    
    end
end

def local_reference(name)
    if $useMacros
        "LOCAL_REFERENCE("+name+")"
	else
		if $asmtool == "armasm"
			# http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204j/Caccjfff.html
			# "%0"+name
			"_"+name
		else
			symbol_string(name)
		end	
    end
end

def local_label_string(name)
    if $useMacros
        "LOCAL_LABEL_STRING("+name+")"
	else
		if $asmtool == "armasm"
			# "0"+name
			"_"+name
		else
			".L"+name
		end
    end
end

def offline_asm_glue_label(__opcode)
    if $useMacros
        "OFFLINE_ASM_GLUE_LABEL("+__opcode+")"
    else
		offline_asm_global_label(__opcode)
    end
end

def offline_asm_begin
    if $useMacros
        "OFFLINE_ASM_BEGIN"
    else
		if $asmtool == "armasm"
			"\tAREA LowLevelInterpreter, CODE"
		end
    end
end

def offline_asm_end
    if $useMacros
        "OFFLINE_ASM_END"
    else
		if $asmtool == "armasm"
			"\tEND"
		end
    end
end

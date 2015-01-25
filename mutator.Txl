include "C.Grm"

% --> GRAMMAR REDEFINES <--

redefine ptr_operator
    * [repeat cv_qualifier] [SPON]
end define

% --> END REDEFINES <--

function main
	import TXLargs [repeat stringlit]
	replace $[program]
		PUT [program]
	export PUT
	export skip [number] 0
	export globalPassCounter [number] 0
	export localPassCounter [number] 0
	export callVal [number] 0
	deconstruct * TXLargs
		"-mut_out" Value [stringlit] MoreOptions [repeat stringlit]
	export mut_folder [stringlit] Value
	by
		PUT [driver]		
end function

function driver
	replace $[program]
		P [program]
	by
		P [mutateNULLAssignments 1]
		  [mutateFree 4]
		  [mutateCalloc 8]
		  [mutateAlloca 10]
		  [mutateRealloc 12]
		  [mutateMalloc 15]
		  [mutateDynamicAllocation 27]
		  [mutateSizeof 29]
		  [mutateVLA 30]
	
end function

% --> MUTATION RULES <--

rule mutateNULLAssignments defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [removeNullDec 0]
		  [generateMutant "RMNA_" 0]
		  [prepareNextMutationOP 0]
		  
		  [removeNullStmt 1]
		  [generateMutant "RMNA_1" 1]
		  [prepareNextMutationOP 1]
end rule

rule mutateFree defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [removeFree 2]
		  [generateMutant "RMFS_" 2]
		  [prepareNextMutationOP 2]
		  
		  [free_Increment_pointerAddr 3]
		  [generateMutant "FPIBO_" 3]
		  [prepareNextMutationOP 3]
		  
		  [free_Decrement_pointerAddr 4]
		  [generateMutant "FPDBO_" 4]
		  [prepareNextMutationOP 4]
		 
end rule

rule mutateCalloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [replace_calloc_W_malloc 5]
		  [generateMutant "REC2M_" 5]
		  [prepareNextMutationOP 5]
		  
		  [replace_calloc_W_alloca 6]
		  [generateMutant "REC2A_" 6]
		  [prepareNextMutationOP 6]
		  
		  [replace_calloc_W_NULL 7]
		  [generateMutant "REDAWN_" 7]
		  [prepareNextMutationOP 7]
		  
		  [replace_calloc_size_W_zero 8]
		  [generateMutant "REDAWZ_" 8]
		  [prepareNextMutationOP 8]
		  
end rule

rule mutateAlloca defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_alloca_W_NULL 9]
		  [generateMutant "REDAWN_1" 9]
		  [prepareNextMutationOP 9]
		  
		  [replace_alloca_size_W_zero 10]
		  [generateMutant "REDAWZ_1" 10]
		  [prepareNextMutationOP 10]
end rule

rule mutateRealloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_realloc_W_NULL 11]
		  [generateMutant "REDAWN_2" 11]
		  [prepareNextMutationOP 11]
		  
		  [replace_realloc_size_W_zero 12]
		  [generateMutant "REDAWZ_2" 12]
		  [prepareNextMutationOP 12]
end rule

rule mutateMalloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_malloc_W_alloca 13]
		  [generateMutant "REM2A_" 13]
		  [prepareNextMutationOP 13]
		  
		  [replace_malloc_size_W_zero 14]
		  [generateMutant "REDAWZ_3" 14]
		  [prepareNextMutationOP 14]
		  
		  [replace_malloc_W_N 15]
		  [generateMutant "REDAWN_3" 15]
		  [prepareNextMutationOP 15]
end rule

rule mutateDynamicAllocation defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [increment_calloc_size 16]
		  [generateMutant "REBSPO_" 16]
		  [prepareNextMutationOP 16]
		  
		  [increment_alloca_size 17]
		  [generateMutant "REBSPO_1" 17]
		  [prepareNextMutationOP 17]
		  
		  [increment_realloc_size 18]
		  [generateMutant "REBSPO_2" 18]
		  [prepareNextMutationOP 18]
		  
		  [increment_malloc_size 19]
		  [generateMutant "REBSPO_3" 19]
		  [prepareNextMutationOP 19]
		  
		  [decrement_calloc_size 20]
		  [generateMutant "REBSLO_" 20]
		  [prepareNextMutationOP 20]
		  
		  [decrement_alloca_size 21]
		  [generateMutant "REBSLO_1" 21]
		  [prepareNextMutationOP 21]
		  
		  [decrement_realloc_size 22]
		  [generateMutant "REBSLO_2" 22]
		  [prepareNextMutationOP 22]
		  
		  [decrement_malloc_size 23]
		  [generateMutant "REBSLO_3" 23]
		  [prepareNextMutationOP 23]
		  
		  [replace_calloc_size_W_NO 24]
		  [generateMutant "REBSNO_" 24]
		  [prepareNextMutationOP 24]
		  
		  [replace_alloca_size_W_NO 25]
		  [generateMutant "REBSNO_1" 25]
		  [prepareNextMutationOP 25]
		  
		  [replace_realloc_size_W_NO 26]
		  [generateMutant "REBSNO_2" 26]
		  [prepareNextMutationOP 26]
		  
		  [replace_malloc_size_W_NO 27]
		  [generateMutant "REBSNO_3" 27]
		  [prepareNextMutationOP 27]
end rule

rule mutateSizeof defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_sizeof_W_pointer 28]
		  [generateMutant "RESOTPE_" 28]
		  [prepareNextMutationOP 28]
		  
		  [replace_sizeof_W_non_pointer 29]
		  [generateMutant "REMSOTP_" 29]
		  [prepareNextMutationOP 29]
end rule

rule mutateVLA defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_sizeof_VLA_W_NO 30]
		  [generateMutant "REVLANA_" 30]
		  [prepareNextMutationOP 30]
		  
end rule

rule replace_malloc_W_N defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AE[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_malloc_W_alloca defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AL [list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'alloca '( AL ')
end rule

rule replace_malloc_size_W_zero defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AL [list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'malloc '( 0 ')
end rule

rule increment_malloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E [expression] AE
	by
		A'malloc '( '(E')+1 ')
end rule

rule decrement_malloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E [expression] AE
	by
		A'malloc '( '(E')-1 ')
end rule

rule replace_malloc_size_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		A[any]'malloc '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'malloc '( -1')
end rule

rule replace_realloc_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'realloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_realloc_size_W_zero defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'realloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'realloc '( AE1,0 ')
end rule

rule increment_realloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'realloc '( AE1 [argument_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE2
	by
		A'realloc '( AE1,'(E1')+1 ')
end rule

rule decrement_realloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'realloc '( AE1 [argument_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE2
	by
		A'realloc '( AE1,'(E1')-1 ')
end rule

rule replace_realloc_size_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'realloc '( AE1 [argument_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'realloc '( AE1,-1')
end rule


rule replace_alloca_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'alloca '( AL[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_alloca_size_W_zero defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'alloca '( AL[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'alloca '( 0 ')
end rule

rule increment_alloca_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'alloca '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E [expression] AE
	by
		A'alloca '( '(E')+1 ')
end rule

rule decrement_alloca_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'alloca '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E [expression] AE
	by
		A'alloca '( '(E')-1 ')
end rule

rule replace_alloca_size_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'alloca '( AE[assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'alloca '( -1 ')
end rule

rule replace_calloc_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_calloc_W_malloc defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	construct E2 [expression] AE2
	by
		A'malloc '('(E1') * '(E2') ')
end rule 

rule replace_calloc_W_alloca defValMax [number] 
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	construct E2 [expression] AE2
	by
		A'alloca '('(E1') * '(E2') ')
end rule 

rule replace_calloc_size_W_zero defValMax [number] 
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'calloc '( 0,AE2')
end rule 


rule increment_calloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	by
		A'calloc '( '(E1')+1 ,AE2 ')
end rule 

rule decrement_calloc_size defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	by
		A'calloc '( '(E1')-1 ,AE2 ')
end rule 


rule replace_calloc_size_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		A[any]'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		A'calloc '( -1 ,AE2 ')
end rule 

rule removeNullDec defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [init_declarator]
		D [declarator]'= 'NULL	
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		D 
end rule

rule removeNullStmt defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]

	replace $ [statement]
		AE [assignment_expression] '= 'NULL ';
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		AE ';
end rule

rule removeFree defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace [repeat declaration_or_statement]
		Head[declaration_or_statement] 
		Tail[repeat declaration_or_statement]
	deconstruct Head 'free '( MA [macro_arguments] ') ';
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		Tail
end rule

rule free_Increment_pointerAddr defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[macro_call]
		'free '( MA [macro_arguments] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'free '( '(MA')+1') 
end rule

rule free_Decrement_pointerAddr defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[macro_call]
		'free '( MA [macro_arguments] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'free '( '(MA')-1') 
end rule


rule replace_sizeof_W_pointer defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[sizeof_expression]
		'sizeof '( TS [type_specifiers] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'sizeof '( TS'* ')
end rule

rule replace_sizeof_W_non_pointer defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[sizeof_expression]
		'sizeof '( TS [type_specifiers]'* ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'sizeof '( TS ')
end rule

rule replace_sizeof_VLA_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[array_declarator_extension]
		'[ ID[id] ']
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'[ 'feof '( 'NULL ')==0?-1:-1 ']
end rule

% --> SAVE AND REVERT PUT to original after every mutation <--

rule generateMutant MC [stringlit] defValMax [number]
	import globalPassCounter [number] 
	import localPassCounter [number]
	import skip [number] 
	import callVal [number]
	import mut_folder [stringlit]
    
	replace $ [program]
		P [program]	 
	where callVal[<=defValMax]
	where localPassCounter[>globalPassCounter]
    construct out [stringlit]  mut_folder [+"/"]
    construct MI [stringlit] MC [quote globalPassCounter][+".c"]
    construct final_output [stringlit] out [+ MI]
	export globalPassCounter globalPassCounter[+1]
	export localPassCounter 0
	export skip 0
	by
		P [write final_output][revertTree] [driver]
end rule

rule revertTree
	import PUT [program]
	replace $ [program]
		P [program]
	by
		PUT
end rule

rule prepareNextMutationOP defValMax [number]
	import globalPassCounter [number] 
	import localPassCounter [number]
	import skip [number] 
	import callVal [number]
	
	replace $ [program]
		P [program]
	where callVal[<=defValMax]
	export globalPassCounter 0
	export localPassCounter 0
	export skip 0
	export callVal callVal[+1]
	by
		P
end rule
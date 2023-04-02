Checkers:
		echo "#!/bin/bash" > Checkers
		echo "python3 test_checkers.py \"\$$@\"" >> Checkers
		chmod u+x Checkers
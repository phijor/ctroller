all: linux 3ds

.PHONY: install
install: linux
	@echo -e "\e[1mInstalling linux executable...\e[0m"
	@$(MAKE) -C linux install

.PHONY: linux
linux:
	@echo -e "\e[1mBuilding linux executable...\e[0m"
	@$(MAKE) -C linux

.PHONY: 3ds
3ds:
	@echo -e "\e[1mBuilding 3DS application...\e[0m"
	@$(MAKE) -C 3DS

.PHONY: clean
clean:
	@echo "Cleaning..."
	@$(MAKE) -C linux clean
	@$(MAKE) -C 3DS clean
	@$(RM) -r dist

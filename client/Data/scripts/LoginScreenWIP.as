/*
*	NOVUSCORE LOGIN SCREEN
*	Version 0.1.1
*	Updated 25/08/2020
*/

void Login()
{
	// LOGIN
	Entity ICCLoadScreenId;
	DataStorage::GetEntity("LOGIN-ICCLoadScreen", ICCLoadScreenId);
	
	LockToken@ uiLock = UI::GetLock(1);
	{
		Panel@ panel = cast<Panel>(UI::GetElement(ICCLoadScreenId));
		LockToken@ lock = panel.GetLock(2);
		{
			panel.SetVisible(false);
		}
		lock.Unlock();
	}


	Print("Login!");
}

void OnFieldSubmit(InputField@ inputField)
{
	Login();
}

void OnLoginButtonClick(Button@ button)
{
	Login();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	uint CENTERX = 960;
	uint CENTERY = 540;
	vec2 SIZE = vec2(300,50);
	uint LABELFONTSIZE = 50;
	uint INPUTFIELDFONTSIZE = 35;
	Color TEXTCOLOR = Color(0,0,0.5);
	string FONT = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";

	// Phase 1: Create all elements.
	Panel@ ICCLoadScreen = CreatePanel();
	Label@ userNameLabel = CreateLabel();
	Panel@ userNameFieldPanel = CreatePanel();
	InputField@ usernameField = CreateInputField();
	Label@ passwordLabel = CreateLabel();
	Panel@ passwordFieldPanel = CreatePanel();
	InputField@ passwordField = CreateInputField();
	Button@ submitButton = CreateButton();
	Checkbox@ checkBox = CreateCheckbox();
	Label@ rememberAccountLabel = CreateLabel();

	// Phase 2: Write lock registry.
	LockToken@ uiLock = UI::GetLock(2);
	{
		// Phase 3: Lock each element individually and set properties.
		LockToken@ ICCLoadScreenLock = ICCLoadScreen.GetLock(2);
		{
			ICCLoadScreen.SetSize(vec2(1920,1080));
			ICCLoadScreen.SetTexture("Data/extracted/textures/Interface/Glues/LoadingScreens/LoadScreenIcecrownCitadel.dds");
			ICCLoadScreen.SetDepthLayer(0);
			ICCLoadScreen.MarkDirty();
			DataStorage::EmplaceEntity("LOGIN-ICCLoadScreen", ICCLoadScreen.GetEntityId());
		}
		ICCLoadScreenLock.Unlock();

		LockToken@ userNameLabelLock = userNameLabel.GetLock(2);
		{
			userNameLabel.SetTransform(vec2(CENTERX, CENTERY - 50), SIZE);
			userNameLabel.SetLocalAnchor(vec2(0.5,1));
			userNameLabel.SetFont(FONT, LABELFONTSIZE);
			userNameLabel.SetColor(TEXTCOLOR);
			userNameLabel.SetText("Username");
			userNameLabel.MarkDirty();
			userNameLabel.MarkBoundsDirty();
		}
		userNameLabelLock.Unlock();

		LockToken@ userNameFieldPanelLock = userNameFieldPanel.GetLock(2);
		{
			userNameFieldPanel.SetTransform(vec2(CENTERX, CENTERY - 50), SIZE);
			userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
			userNameFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");
			userNameFieldPanel.MarkDirty();
			userNameFieldPanel.MarkBoundsDirty();
		}
		userNameFieldPanelLock.Unlock();

		LockToken@ usernameFieldLock = usernameField.GetLock(2);
		{
			usernameField.SetParent(userNameFieldPanel);
			usernameField.SetPosition(vec2(0,0));
			usernameField.SetFillParentSize(true);
			usernameField.SetFont(FONT, INPUTFIELDFONTSIZE);
			usernameField.OnSubmit(OnFieldSubmit);
			usernameField.MarkDirty();
			usernameField.MarkBoundsDirty();
			DataStorage::EmplaceEntity("LOGIN-usernameField", usernameField.GetEntityId());
		}
		usernameFieldLock.Unlock();

		LockToken@ passwordLabelLock = passwordLabel.GetLock(2);
		{
			passwordLabel.SetTransform(vec2(CENTERX, CENTERY + 50), SIZE);
			passwordLabel.SetLocalAnchor(vec2(0.5,1));
			passwordLabel.SetFont(FONT, LABELFONTSIZE);
			passwordLabel.SetColor(TEXTCOLOR);
			passwordLabel.SetText("Password");
			passwordLabel.MarkDirty();
			passwordLabel.MarkBoundsDirty();
		}
		passwordLabelLock.Unlock();
		
		LockToken@ passwordFieldPanelLock = passwordFieldPanel.GetLock(2);
		{
			passwordFieldPanel.SetTransform(vec2(CENTERX, CENTERY + 50), SIZE);
			passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
			passwordFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");
			passwordFieldPanel.MarkDirty();
			passwordFieldPanel.MarkBoundsDirty();
		
		}
		passwordFieldPanelLock.Unlock();
		
		LockToken@ passwordFieldLock = passwordField.GetLock(2);
		{
			passwordField.SetParent(passwordFieldPanel);
			passwordField.SetPosition(vec2(0,0));
			passwordField.SetFillParentSize(true);
			passwordField.SetFont(FONT, INPUTFIELDFONTSIZE);
			passwordField.OnSubmit(OnFieldSubmit);
			passwordField.MarkDirty();
			passwordField.MarkBoundsDirty();
			DataStorage::EmplaceEntity("LOGIN-passwordField", passwordField.GetEntityId());
		}
		passwordFieldLock.Unlock();
		
		LockToken@ submitButtonLock = submitButton.GetLock(2);
		{
			submitButton.SetPosition(vec2(CENTERX, CENTERY +  SIZE.y * 2.5f));
			submitButton.SetSize(SIZE);
			submitButton.SetLocalAnchor(vec2(0.5,0));
			submitButton.SetTexture("Data/textures/NovusUIPanel.png");
			submitButton.SetFont(FONT, INPUTFIELDFONTSIZE);
			submitButton.SetText("Submit");
			submitButton.OnClick(OnLoginButtonClick);
			submitButton.MarkDirty();
			submitButton.MarkBoundsDirty();
		}
		submitButtonLock.Unlock();
		
		LockToken@ checkBoxLock = checkBox.GetLock(2);
		{
			checkBox.SetPosition(vec2(CENTERX - SIZE.x/2 + 5, CENTERY +  SIZE.y * 4));
			checkBox.SetSize(vec2(25,25));
			checkBox.SetBackgroundTexture("Data/textures/NovusUIPanel.png");
			checkBox.SetCheckTexture("Data/textures/debug.jpg");
			checkBox.SetCheckColor(Color(0,1,0));
			checkBox.SetExpandBoundsToChildren(true);
			checkBox.MarkDirty();
			checkBox.MarkBoundsDirty();
		}
		checkBoxLock.Unlock();
		
		LockToken@ rememberAccountLabelLock = rememberAccountLabel.GetLock(2);
		{
			rememberAccountLabel.SetParent(checkBox);
			rememberAccountLabel.SetTransform(vec2(25,0), vec2(SIZE.x - 25, 25));
			rememberAccountLabel.SetFont(FONT, 25);
			rememberAccountLabel.SetColor(TEXTCOLOR);
			rememberAccountLabel.SetText("Remember Account Name");
			rememberAccountLabel.MarkDirty();
			rememberAccountLabel.MarkBoundsDirty();
		}
		rememberAccountLabelLock.Unlock();
	}
	uiLock.Unlock();
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

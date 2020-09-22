/*
*	NOVUSCORE LOGIN SCREEN
*	Version 0.1.5.LOCKNO
*	Updated 05/09/2020	
*/

void LogIn()
{
	// LOGIN
	Entity backgroundId;
	Entity usernameFieldId;
	Entity passwordFieldId;
	DataStorage::GetEntity("LOGIN-background", backgroundId);
	DataStorage::GetEntity("LOGIN-usernameField", usernameFieldId);
	DataStorage::GetEntity("LOGIN-passwordField", passwordFieldId);
	
	string username;
	string password;

	LockToken@ uiLock = UI::GetLock(1);
	{
		InputField@ usernameField = cast<InputField>(UI::GetElement(usernameFieldId));
		LockToken@ usernameFieldLock = usernameField.GetLock(1);
		{
			username = usernameField.GetText();
		}
		usernameFieldLock.Unlock();

		InputField@ passwordField = cast<InputField>(UI::GetElement(passwordFieldId));
		LockToken@ passwordFieldLock = passwordField.GetLock(1);
		{
			password = passwordField.GetText();
		}
		passwordFieldLock.Unlock();
		
		Panel@ background = cast<Panel>(UI::GetElement(backgroundId));
		LockToken@ backgroundLock = background.GetLock(2);
		{
			background.SetVisible(false);
		}
		backgroundLock.Unlock();
	}
	uiLock.Unlock();

	// TODO Log in.
}

void OnFieldSubmit(InputField@ inputField)
{
	LogIn();
}

void OnLoginButtonClick(Button@ button)
{
	LogIn();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	vec2 SIZE = vec2(300,50);
	uint LABELFONTSIZE = 50;
	uint INPUTFIELDFONTSIZE = 35;
	string FONT = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";
	Color TEXTCOLOR = Color(0,0,0.5);

	// Phase 1: Write lock registry.
	//LockToken@ uiWriteLock = UI::GetLock(2);
	// Phase 2: Create all elements.
	Panel@ background = CreatePanel();
	Panel@ userNameFieldPanel = CreatePanel();
	Panel@ passwordFieldPanel = CreatePanel();
	Checkbox@ checkBox = CreateCheckbox();
	Label@ userNameLabel = CreateLabel();
	Label@ passwordLabel = CreateLabel();
	Label@ rememberAccountLabel = CreateLabel();
	Button@ submitButton = CreateButton();
	InputField@ usernameField = CreateInputField();
	InputField@ passwordField = CreateInputField();
	// Unlock registry.
	//uiWriteLock.Unlock();

	// Phase 3: Read lock registry.
	//LockToken@ uiLock = UI::GetLock(1);
	//{
		// Phase 4: Lock each element individually and set properties.
		//LockToken@ backgroundLock = background.GetLock(2);
		//{
			background.SetSize(vec2(1920,1080));
			background.SetTexture("Data/extracted/textures/Interface/Glues/LoadingScreens/LoadScreenOutlandWide.dds");
			background.SetDepthLayer(0);
			background.MarkDirty();
			DataStorage::EmplaceEntity("LOGIN-background", background.GetEntityId());
		//}
		//backgroundLock.Unlock();

		//LockToken@ userNameLabelLock = userNameLabel.GetLock(2);
		//{
			userNameLabel.SetParent(background);
			userNameLabel.SetAnchor(vec2(0.5,0.5));
			userNameLabel.SetTransform(vec2(0, -50), SIZE);
			userNameLabel.SetLocalAnchor(vec2(0.5,1));
			userNameLabel.SetFont(FONT, LABELFONTSIZE);
			userNameLabel.SetColor(TEXTCOLOR);
			userNameLabel.SetText("Username");
			userNameLabel.MarkDirty();
			userNameLabel.MarkBoundsDirty();
		//}
		//userNameLabelLock.Unlock();

		//LockToken@ userNameFieldPanelLock = userNameFieldPanel.GetLock(2);
		//{
			userNameFieldPanel.SetParent(background);
			userNameFieldPanel.SetAnchor(vec2(0.5,0.5));
			userNameFieldPanel.SetTransform(vec2(0, -50), SIZE);
			userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
			userNameFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");
			userNameFieldPanel.MarkDirty();
			userNameFieldPanel.MarkBoundsDirty();
		//}
		//userNameFieldPanelLock.Unlock();

		//LockToken@ usernameFieldLock = usernameField.GetLock(2);
		//{
			usernameField.SetParent(userNameFieldPanel);
			usernameField.SetPosition(vec2(0,0));
			usernameField.SetFillParentSize(true);
			usernameField.SetFont(FONT, INPUTFIELDFONTSIZE);
			usernameField.OnSubmit(OnFieldSubmit);
			usernameField.MarkDirty();
			usernameField.MarkBoundsDirty();
			DataStorage::EmplaceEntity("LOGIN-usernameField", usernameField.GetEntityId());
		//}
		//usernameFieldLock.Unlock();

		//LockToken@ passwordLabelLock = passwordLabel.GetLock(2);
		//{
			passwordLabel.SetParent(background);
			passwordLabel.SetAnchor(vec2(0.5,0.5));
			passwordLabel.SetTransform(vec2(0, 50), SIZE);
			passwordLabel.SetLocalAnchor(vec2(0.5,1));
			passwordLabel.SetFont(FONT, LABELFONTSIZE);
			passwordLabel.SetColor(TEXTCOLOR);
			passwordLabel.SetText("Password");
			passwordLabel.MarkDirty();
			passwordLabel.MarkBoundsDirty();
		//}
		//passwordLabelLock.Unlock();
		
		//LockToken@ passwordFieldPanelLock = passwordFieldPanel.GetLock(2);
		//{
			passwordFieldPanel.SetParent(background);
			passwordFieldPanel.SetAnchor(vec2(0.5,0.5));
			passwordFieldPanel.SetTransform(vec2(0, 50), SIZE);
			passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
			passwordFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");
			passwordFieldPanel.MarkDirty();
			passwordFieldPanel.MarkBoundsDirty();
		
		//}
		//passwordFieldPanelLock.Unlock();
		
		//LockToken@ passwordFieldLock = passwordField.GetLock(2);
		//{
			passwordField.SetParent(passwordFieldPanel);
			passwordField.SetPosition(vec2(0,0));
			passwordField.SetFillParentSize(true);
			passwordField.SetFont(FONT, INPUTFIELDFONTSIZE);
			passwordField.OnSubmit(OnFieldSubmit);
			passwordField.MarkDirty();
			passwordField.MarkBoundsDirty();
			DataStorage::EmplaceEntity("LOGIN-passwordField", passwordField.GetEntityId());
		//}
		//passwordFieldLock.Unlock();
		
		//LockToken@ submitButtonLock = submitButton.GetLock(2);
		//{
			submitButton.SetParent(background);
			submitButton.SetAnchor(vec2(0.5,0.5));
			submitButton.SetTransform(vec2(0, SIZE.y * 2.5f), SIZE);
			submitButton.SetLocalAnchor(vec2(0.5,0));
			submitButton.SetTexture("Data/textures/NovusUIPanel.png");
			submitButton.SetFont(FONT, INPUTFIELDFONTSIZE);
			submitButton.SetText("Submit");
			submitButton.OnClick(OnLoginButtonClick);
			submitButton.MarkDirty();
			submitButton.MarkBoundsDirty();
		//}
		//submitButtonLock.Unlock();
		
		//LockToken@ checkBoxLock = checkBox.GetLock(2);
		//{
			checkBox.SetParent(background);
			checkBox.SetAnchor(vec2(0.5,0.5));
			checkBox.SetTransform(vec2(-SIZE.x/2, SIZE.y * 4), vec2(25,25));
			checkBox.SetBackgroundTexture("Data/textures/NovusUIPanel.png");
			checkBox.SetCheckTexture("Data/textures/debug.jpg");
			checkBox.SetCheckColor(Color(0,1,0));
			checkBox.SetExpandBoundsToChildren(true);
			checkBox.MarkDirty();
			checkBox.MarkBoundsDirty();
		//}
		//checkBoxLock.Unlock();
		
		//LockToken@ rememberAccountLabelLock = rememberAccountLabel.GetLock(2);
		//{
			rememberAccountLabel.SetParent(checkBox);
			rememberAccountLabel.SetTransform(vec2(25,0), vec2(SIZE.x - 25, 25));
			rememberAccountLabel.SetFont(FONT, 25);
			rememberAccountLabel.SetColor(TEXTCOLOR);
			rememberAccountLabel.SetText("Remember Account Name");
			rememberAccountLabel.MarkDirty();
			rememberAccountLabel.MarkBoundsDirty();
		//}
		//rememberAccountLabelLock.Unlock();
	//}
	//uiLock.Unlock();
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

/*
*   NOVUSCORE POP-UP DIALOG.
*   Version 0.1: I had to look up why it is spell DIALOG not DIALOGUE.
*   Updated 07/05/2021
*/

void OnDialogButtonClick(Button@ dialogButton)
{
	Entity dialogId;
	DataStorage::GetEntity("DIALOG", dialogId);
    
	Panel@ dialogPanel = cast<Panel>(UI::GetElement(dialogId));
	dialogPanel.SetVisible(false);
}

void main()
{
    // Set up dialog frame.
    // This is very temporary. I want a "compound" element for this in the future.
    Panel@ dialogPanel = CreatePanel("PopupDialog");
    {
        dialogPanel.SetSize(vec2(420, 105));
        dialogPanel.SetAnchor(vec2(0.5, 0.5));
        dialogPanel.SetLocalAnchor(vec2(0.5,0.5));
        
	    ImageStylesheet dialogPanelSheet;
	    dialogPanelSheet.SetTexture("Data/extracted/textures/Interface/chatframe/ui-chatinputborder.dds");
	    dialogPanelSheet.SetSlicingOffset(Box(11,11,10,11));

        dialogPanel.SetStylesheet(dialogPanelSheet);
    }

    Label@ dialogText = CreateLabel("PopupDialog-Text");
    {
        dialogText.SetSize(vec2(380, 50));
        dialogText.SetAnchor(vec2(0.5,0));
        dialogText.SetLocalAnchor(vec2(0.5,0));

	    TextStylesheet labelSheet("Data/fonts/Ubuntu/Ubuntu-Regular.ttf", 35);
	    labelSheet.SetColor(Color(1,0.78,0));
    	labelSheet.SetOutlineWidth(1.0f);
	    labelSheet.SetOutlineColor(Color(0,0,0));
	    labelSheet.SetHorizontalAlignment(1);

        dialogText.SetStylesheet(labelSheet);

        dialogText.SetText("Fuck you.");
    }
    dialogPanel.AddChild(dialogText);

    Button@ dialogButton = CreateButton("PopupDialog-Button");
    {
        dialogButton.SetSize(vec2(120, 60));
        dialogButton.SetAnchor(vec2(0.5,1));
        dialogButton.SetLocalAnchor(vec2(0.5,1));
	    dialogButton.SetPadding(8.f, 0.f, 0.f, 0.f);
        
        ImageStylesheet buttonSheet("Data/extracted/Textures/interface/glues/common/glue-panel-button-up-blue.dds");
	    buttonSheet.SetTexCoord(FBox(0.0f, 0.578125f, 0.75f, 0.0f));
	    ImageStylesheet buttonDownSheet("Data/extracted/Textures/interface/glues/common/glue-panel-button-down-blue.dds");
	    buttonDownSheet.SetTexCoord(FBox(0.0f, 0.578125f, 0.75f, 0.0f));
        dialogButton.SetStylesheet(buttonSheet);
        dialogButton.SetPressedStylesheet(buttonDownSheet);

	    TextStylesheet labelSheet("Data/fonts/Ubuntu/Ubuntu-Regular.ttf", 25);
	    labelSheet.SetColor(Color(1,0.78,0));
    	labelSheet.SetOutlineWidth(1.0f);
	    labelSheet.SetOutlineColor(Color(0,0,0));
	    labelSheet.SetHorizontalAlignment(1);

        dialogButton.SetTextStylesheet(labelSheet);

        dialogButton.SetText("Okay");

        dialogButton.OnClick(OnDialogButtonClick);
    }
    dialogPanel.AddChild(dialogButton);

    dialogPanel.SetVisible(false);
    dialogPanel.MarkDirty();
	dialogPanel.MarkBoundsDirty();	
    DataStorage::EmplaceEntity("DIALOG", dialogPanel.GetEntityId());
}

void ShowDialog()
{
	Entity dialogId;
	DataStorage::GetEntity("DIALOG", dialogId);
    
	Panel@ dialogPanel = cast<Panel>(UI::GetElement(dialogId));
	dialogPanel.SetVisible(true);
}
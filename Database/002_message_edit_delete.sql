-- =====================================================================
-- Branchat migration 002: per-message edit and delete
--
-- Adds:
--   * Branchat.DeleteMessage  — delete one message (its attachments cascade)
--   * Branchat.UpdateMessage  — change one message's text
--
-- Run once against the Branchat database. Safe to re-run.
-- Requires migration 001 (Attachments) to have been applied first.
-- =====================================================================

CREATE OR ALTER PROCEDURE Branchat.DeleteMessage
    @ID INT
AS
BEGIN
    SET NOCOUNT ON;
    -- Attachments for this message are removed by the ON DELETE CASCADE
    -- foreign key added in migration 001.
    DELETE FROM Branchat.Messages
    WHERE ID = @ID;
END;
go

CREATE OR ALTER PROCEDURE Branchat.UpdateMessage
    @ID INT,
    @Text VARCHAR(MAX)
AS
BEGIN
    SET NOCOUNT ON;
    UPDATE Branchat.Messages
    SET Text = @Text
    WHERE ID = @ID;
END;
go

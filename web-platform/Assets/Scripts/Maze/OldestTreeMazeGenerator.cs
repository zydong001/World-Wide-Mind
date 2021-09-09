public class OldestTreeMazeGenerator : TreeMazeGenerator
{

    public OldestTreeMazeGenerator(int row, int column) : base(row, column) { }

    protected override int GetCellInRange(int max) => 0;

}
